#include "AudioPlayer.h"
#include <iostream>
#include <cstring>
#include <QMutexLocker>

AudioPlayer::AudioPlayer(QObject* parent)
    : QObject(parent), stream(nullptr), playing(false), paused(false), current_position(0) {
    if (!initializePortAudio()) {
        std::cerr << "Failed to initialize PortAudio" << std::endl;
    }
}

AudioPlayer::~AudioPlayer() {
    stopPlayback();
    cleanupPortAudio();
}

bool AudioPlayer::initializePortAudio() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    return true;
}

void AudioPlayer::cleanupPortAudio() {
    if (stream) {
        Pa_CloseStream(stream);
        stream = nullptr;
    }
    Pa_Terminate();
}

bool AudioPlayer::loadFile(const std::string& filename) {
    stopPlayback();
    current_position = 0;
    return decoder.loadFile(filename);
}

bool AudioPlayer::startPlayback() {
    if (!decoder.isLoaded()) {
        std::cerr << "No file loaded" << std::endl;
        return false;
    }
    
    if (playing && !paused) {
        return true; // Already playing
    }
    
    if (paused) {
        resumePlayback();
        return true;
    }
    
    // Reset position if at end
    if (current_position >= decoder.getSamples().size()) {
        current_position = 0;
    }
    
    // Get audio parameters
    unsigned int sample_rate = decoder.getSampleRate();
    unsigned int channels = decoder.getChannels();
    
    // Configure PortAudio stream
    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        std::cerr << "No default output device" << std::endl;
        return false;
    }
    
    outputParameters.channelCount = channels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;
    
    // Open stream
    PaError err = Pa_OpenStream(
        &stream,
        nullptr, // No input
        &outputParameters,
        sample_rate,
        paFramesPerBufferUnspecified, // Let PortAudio choose
        0, // No flags
        audioCallback,
        this // User data
    );
    
    if (err != paNoError) {
        std::cerr << "PortAudio stream open error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    
    // Reset FFT analyzer and filter
    fft_analyzer.reset();
    frequency_filter.reset();
    
    // Update filter sample rate
    frequency_filter.setLowPassCutoff(0, sample_rate);
    frequency_filter.setHighPassCutoff(0, sample_rate);
    
    // Start stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream start error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        stream = nullptr;
        return false;
    }
    
    playing = true;
    paused = false;
    return true;
}

void AudioPlayer::stopPlayback() {
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        stream = nullptr;
    }
    playing = false;
    paused = false;
    current_position = 0;
    fft_analyzer.reset();
    frequency_filter.reset();
}

void AudioPlayer::pausePlayback() {
    if (playing && stream) {
        PaError err = Pa_StopStream(stream);
        if (err == paNoError) {
            paused = true;
        }
    }
}

void AudioPlayer::resumePlayback() {
    if (paused && stream) {
        PaError err = Pa_StartStream(stream);
        if (err == paNoError) {
            paused = false;
        } else {
            std::cerr << "Failed to resume playback: " << Pa_GetErrorText(err) << std::endl;
        }
    }
}

int AudioPlayer::audioCallback(const void* input, void* output,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void* userData) {
    AudioPlayer* player = static_cast<AudioPlayer*>(userData);
    return player->processAudio(input, output, frameCount);
}

int AudioPlayer::processAudio(const void* input, void* output, unsigned long frameCount) {
    (void)input; // Unused
    
    if (!decoder.isLoaded()) {
        memset(output, 0, frameCount * decoder.getChannels() * sizeof(float));
        return paComplete;
    }
    
    const std::vector<float>& samples = decoder.getSamples();
    float* out = (float*)output;
    unsigned int channels = decoder.getChannels();
    
    // Check if we've reached the end
    if (current_position >= samples.size()) {
        memset(output, 0, frameCount * channels * sizeof(float));
        playing = false;
        emit playbackFinished();
        return paComplete;
    }
    
    // Copy samples to output buffer
    size_t samples_to_copy = std::min((size_t)frameCount, samples.size() - current_position);
    
    // Get sample rate for filter/visualization
    unsigned int sample_rate = decoder.getSampleRate();
    
    for (size_t i = 0; i < samples_to_copy; i++) {
        float sample = samples[current_position + i];

        // Feed original sample to FFT analyzer (for visualization)
        if (fft_analyzer.addSample(sample)) {
            std::vector<float> magnitudes = fft_analyzer.getMagnitudes();
            if (!magnitudes.empty()) {
                QMutexLocker locker(&filter_mutex);
                // Apply current filter settings to visualization magnitudes
                frequency_filter.processFFT(magnitudes, sample_rate);
            }
            emit fftDataReady(magnitudes);
        }

        // Apply FIR filter in time-domain for audio
        QMutexLocker locker(&filter_mutex);
        float filtered_sample = frequency_filter.processSample(sample);

        // Output filtered sample to all channels (mono or stereo)
        for (unsigned int ch = 0; ch < channels; ch++) {
            out[i * channels + ch] = filtered_sample;
        }
    }
    
    // Zero out remaining frames if we've reached the end
    if (samples_to_copy < frameCount) {
        memset(&out[samples_to_copy * channels], 0, 
               (frameCount - samples_to_copy) * channels * sizeof(float));
    }
    
    current_position += samples_to_copy;
    
    return paContinue;
}

void AudioPlayer::setLowPassCutoff(float cutoffHz) {
    QMutexLocker locker(&filter_mutex);
    unsigned int sample_rate = decoder.getSampleRate();
    if (sample_rate > 0) {
        frequency_filter.setLowPassCutoff(cutoffHz, sample_rate);
    }
}

void AudioPlayer::setHighPassCutoff(float cutoffHz) {
    QMutexLocker locker(&filter_mutex);
    unsigned int sample_rate = decoder.getSampleRate();
    if (sample_rate > 0) {
        frequency_filter.setHighPassCutoff(cutoffHz, sample_rate);
    }
}

void AudioPlayer::setBandStop(float lowHz, float highHz) {
    QMutexLocker locker(&filter_mutex);
    unsigned int sample_rate = decoder.getSampleRate();
    if (sample_rate > 0) {
        frequency_filter.setBandStop(lowHz, highHz, sample_rate);
    }
}

void AudioPlayer::setBandPass(float lowHz, float highHz) {
    QMutexLocker locker(&filter_mutex);
    unsigned int sample_rate = decoder.getSampleRate();
    if (sample_rate > 0) {
        frequency_filter.setBandPass(lowHz, highHz, sample_rate);
    }
}

void AudioPlayer::enableLowPass(bool enabled) {
    QMutexLocker locker(&filter_mutex);
    frequency_filter.enableLowPass(enabled);
}

void AudioPlayer::enableHighPass(bool enabled) {
    QMutexLocker locker(&filter_mutex);
    frequency_filter.enableHighPass(enabled);
}

void AudioPlayer::enableBandStop(bool enabled) {
    QMutexLocker locker(&filter_mutex);
    frequency_filter.enableBandStop(enabled);
}

void AudioPlayer::enableBandPass(bool enabled) {
    QMutexLocker locker(&filter_mutex);
    frequency_filter.enableBandPass(enabled);
}

