#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QThread>
#include <vector>
#include <string>
#include <portaudio.h>
#include "AudioDecoder.h"
#include "FFTAnalyzer.h"

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
    size_t getCurrentPosition() const { return current_position; }
    
    // Get total length (in samples)
    size_t getTotalLength() const { return decoder.isLoaded() ? decoder.getSamples().size() : 0; }

signals:
    // Signal emitted when new FFT data is available
    void fftDataReady(const std::vector<float>& magnitudes);
    
    // Signal emitted when playback finishes
    void playbackFinished();

private:
    AudioDecoder decoder;
    FFTAnalyzer fft_analyzer;
    
    PaStream* stream;
    bool playing;
    bool paused;
    size_t current_position;
    
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

