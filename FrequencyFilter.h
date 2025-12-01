#ifndef FREQUENCYFILTER_H
#define FREQUENCYFILTER_H

#include <vector>
#include <cmath>
#include <algorithm>
#include <fftw3.h>

class FrequencyFilter {
public:
    FrequencyFilter();
    ~FrequencyFilter();
    
    // Filter configuration
    void setLowPassCutoff(float cutoffHz, float sampleRate);
    void setHighPassCutoff(float cutoffHz, float sampleRate);
    void setBandStop(float lowHz, float highHz, float sampleRate);
    void setBandPass(float lowHz, float highHz, float sampleRate);
    
    // Enable/disable filters
    void enableLowPass(bool enabled) { lowPassEnabled = enabled; }
    void enableHighPass(bool enabled) { highPassEnabled = enabled; }
    void enableBandStop(bool enabled) { bandStopEnabled = enabled; }
    void enableBandPass(bool enabled) { bandPassEnabled = enabled; }
    
    // Apply filter to a single sample (time-domain filtering)
    float processSample(float sample);
    
    // Apply filter to FFT magnitudes (frequency-domain filtering)
    void processFFT(std::vector<float>& magnitudes, float sampleRate);
    
    // Apply filter to complex FFT data (zeros out filtered bins completely)
    void processComplexFFT(fftw_complex* fftData, int fftSize, float sampleRate);
    
    // Reset filter state
    void reset();
    
    // Check if any filter is active
    bool isActive() const;

private:
    // Filter types
    bool lowPassEnabled;
    bool highPassEnabled;
    bool bandStopEnabled;
    bool bandPassEnabled;
    
    // Filter parameters
    float lowPassCutoff;
    float highPassCutoff;
    float bandStopLow;
    float bandStopHigh;
    float bandPassLow;
    float bandPassHigh;
    float currentSampleRate;
    
    // FIR filter coefficients and state
    std::vector<float> lowPassCoeffs;
    std::vector<float> highPassCoeffs;
    std::vector<float> bandStopCoeffs;
    std::vector<float> bandPassCoeffs;
    
    // Filter delay lines (for FIR filtering)
    std::vector<float> lowPassDelayLine;
    std::vector<float> highPassDelayLine;
    std::vector<float> bandStopDelayLine;
    std::vector<float> bandPassDelayLine;
    
    // Helper methods
    // Sharper FIR filters (longer length for stronger attenuation)
    void generateLowPassCoeffs(float cutoffHz, float sampleRate, int filterLength = 257);
    void generateHighPassCoeffs(float cutoffHz, float sampleRate, int filterLength = 257);
    void generateBandStopCoeffs(float lowHz, float highHz, float sampleRate, int filterLength = 257);
    void generateBandPassCoeffs(float lowHz, float highHz, float sampleRate, int filterLength = 257);
    
    // Window function (Blackman window)
    float blackmanWindow(int n, int N);
    
    // Sinc function
    float sinc(float x);
    
    // Apply FIR filter
    float applyFIR(const std::vector<float>& coeffs, std::vector<float>& delayLine, float sample);
    
    // Convert frequency bin to Hz
    float binToHz(int bin, int fftSize, float sampleRate);
};

#endif // FREQUENCYFILTER_H

