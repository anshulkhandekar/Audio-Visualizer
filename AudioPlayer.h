#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QThread>
#include <vector>
#include <string>
#include <portaudio.h>
#include <QMutex>
#include <QElapsedTimer>
#include "AudioDecoder.h"
#include "FFTAnalyzer.h"
#include "FrequencyFilter.h"
#include "AudioExporter.h"

class AudioPlayer : public QObject {
    Q_OBJECT

public:
    AudioPlayer(QObject* parent = nullptr);
    ~AudioPlayer();
    
    // Load audio file
    bool loadFile(const std::string& filename);
    
    // Playback control
    bool startPlayback();
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    
    // Check if playing
    bool isPlaying() const { return playing; }
    bool isPaused() const { return paused; }
    
    // Get current position (in samples)
    size_t getCurrentPosition() const {
        QMutexLocker locker(&position_mutex);
        return current_position;
    }
    
    // Get total length (in samples)
    size_t getTotalLength() const { return decoder.isLoaded() ? decoder.getSamples().size() : 0; }
    
    // Seek to a specific position (in samples)
    void seekToPosition(size_t position);
    
    // Get sample rate
    unsigned int getSampleRate() const { return decoder.isLoaded() ? decoder.getSampleRate() : 0; }
    
    // Set volume (0.0 to 1.0)
    void setVolume(float volume);
    
    // Export current edited audio to a WAV file (applies current filter settings offline)
    bool exportEditedToWav(const std::string& path);
    
    // Filter control methods
    void setLowPassCutoff(float cutoffHz);
    void setHighPassCutoff(float cutoffHz);
    void setBandStop(float lowHz, float highHz);
    void setBandPass(float lowHz, float highHz);
    void enableLowPass(bool enabled);
    void enableHighPass(bool enabled);
    void enableBandStop(bool enabled);
    void enableBandPass(bool enabled);

signals:
    // Signal emitted when new FFT data is available
    void fftDataReady(const std::vector<float>& magnitudes);
    
    // Signal emitted when playback finishes
    void playbackFinished();
    
    // Signal emitted when position changes (for UI updates)
    void positionChanged(size_t position);

private:
    AudioDecoder decoder;
    FFTAnalyzer fft_analyzer; // For visualization
    FrequencyFilter frequency_filter;
    
    PaStream* stream;
    bool playing;
    bool paused;
    size_t current_position;
    
    // Thread-safe filter parameter updates
    QMutex filter_mutex;
    
    // Thread-safe position updates
    mutable QMutex position_mutex;
    
    // Thread-safe volume updates
    float volume;
    mutable QMutex volume_mutex;
    
    // FFT signal throttling (emit at most every ~33ms = ~30 FPS)
    QElapsedTimer fftEmitTimer;
    static constexpr int FFT_EMIT_INTERVAL_MS = 33;
    
    // PortAudio callback (static, calls instance method)
    static int audioCallback(const void* input, void* output,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData);
    
    // Instance callback method
    int processAudio(const void* input, void* output, unsigned long frameCount);
    
    // Initialize PortAudio
    bool initializePortAudio();
    void cleanupPortAudio();
};

#endif // AUDIOPLAYER_H

