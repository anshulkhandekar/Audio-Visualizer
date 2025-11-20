#include "FrequencyFilter.h"
#include <cstring>
#include <cmath>

FrequencyFilter::FrequencyFilter()
    : lowPassEnabled(false), highPassEnabled(false),
      bandStopEnabled(false), bandPassEnabled(false),
      lowPassCutoff(0.0f), highPassCutoff(0.0f),
      bandStopLow(0.0f), bandStopHigh(0.0f),
      bandPassLow(0.0f), bandPassHigh(0.0f),
      currentSampleRate(44100.0f) {
    // Initialize delay lines with reasonable default size
    lowPassDelayLine.resize(101, 0.0f);
    highPassDelayLine.resize(101, 0.0f);
    bandStopDelayLine.resize(101, 0.0f);
    bandPassDelayLine.resize(101, 0.0f);
}

FrequencyFilter::~FrequencyFilter() {
}

void FrequencyFilter::setLowPassCutoff(float cutoffHz, float sampleRate) {
    lowPassCutoff = cutoffHz;
    currentSampleRate = sampleRate;
    if (cutoffHz > 0 && cutoffHz < sampleRate / 2) {
        generateLowPassCoeffs(cutoffHz, sampleRate);
    }
}

void FrequencyFilter::setHighPassCutoff(float cutoffHz, float sampleRate) {
    highPassCutoff = cutoffHz;
    currentSampleRate = sampleRate;
    if (cutoffHz > 0 && cutoffHz < sampleRate / 2) {
        generateHighPassCoeffs(cutoffHz, sampleRate);
    }
}

void FrequencyFilter::setBandStop(float lowHz, float highHz, float sampleRate) {
    bandStopLow = lowHz;
    bandStopHigh = highHz;
    currentSampleRate = sampleRate;
    if (lowHz > 0 && highHz > lowHz && highHz < sampleRate / 2) {
        generateBandStopCoeffs(lowHz, highHz, sampleRate);
    }
}

void FrequencyFilter::setBandPass(float lowHz, float highHz, float sampleRate) {
    bandPassLow = lowHz;
    bandPassHigh = highHz;
    currentSampleRate = sampleRate;
    if (lowHz > 0 && highHz > lowHz && highHz < sampleRate / 2) {
        generateBandPassCoeffs(lowHz, highHz, sampleRate);
    }
}

float FrequencyFilter::processSample(float sample) {
    float output = sample;
    
    // Apply filters in sequence (order matters for combined filters)
    if (bandPassEnabled && !bandPassCoeffs.empty()) {
        output = applyFIR(bandPassCoeffs, bandPassDelayLine, output);
    }
    
    if (bandStopEnabled && !bandStopCoeffs.empty()) {
        output = applyFIR(bandStopCoeffs, bandStopDelayLine, output);
    }
    
    if (highPassEnabled && !highPassCoeffs.empty()) {
        output = applyFIR(highPassCoeffs, highPassDelayLine, output);
    }
    
    if (lowPassEnabled && !lowPassCoeffs.empty()) {
        output = applyFIR(lowPassCoeffs, lowPassDelayLine, output);
    }
    
    return output;
}

void FrequencyFilter::processFFT(std::vector<float>& magnitudes, float sampleRate) {
    if (magnitudes.empty()) return;
    
    int fftSize = (magnitudes.size() - 1) * 2; // Reconstruct FFT size
    
    for (size_t i = 0; i < magnitudes.size(); i++) {
        float freqHz = binToHz(i, fftSize, sampleRate);
        
        // Apply band-pass filter (keep only frequencies in range)
        if (bandPassEnabled) {
            if (freqHz < bandPassLow || freqHz > bandPassHigh) {
                magnitudes[i] = 0.0f;
                continue;
            }
        }
        
        // Apply band-stop filter (cut frequencies in range)
        if (bandStopEnabled) {
            if (freqHz >= bandStopLow && freqHz <= bandStopHigh) {
                magnitudes[i] = 0.0f;
                continue;
            }
        }
        
        // Apply high-pass filter (cut below cutoff)
        if (highPassEnabled && freqHz < highPassCutoff) {
            magnitudes[i] = 0.0f;
            continue;
        }
        
        // Apply low-pass filter (cut above cutoff)
        if (lowPassEnabled && freqHz > lowPassCutoff) {
            magnitudes[i] = 0.0f;
            continue;
        }
    }
}

void FrequencyFilter::processComplexFFT(fftw_complex* fftData, int fftSize, float sampleRate) {
    if (!fftData || fftSize <= 0) return;
    
    int numBins = fftSize / 2 + 1;
    
    for (int i = 0; i < numBins; i++) {
        float freqHz = binToHz(i, fftSize, sampleRate);
        bool shouldZero = false;
        
        // Apply band-pass filter (keep only frequencies in range)
        if (bandPassEnabled) {
            if (freqHz < bandPassLow || freqHz > bandPassHigh) {
                shouldZero = true;
            }
        }
        
        // Apply band-stop filter (cut frequencies in range)
        if (bandStopEnabled) {
            if (freqHz >= bandStopLow && freqHz <= bandStopHigh) {
                shouldZero = true;
            }
        }
        
        // Apply high-pass filter (cut below cutoff)
        if (highPassEnabled && freqHz < highPassCutoff) {
            shouldZero = true;
        }
        
        // Apply low-pass filter (cut above cutoff)
        if (lowPassEnabled && freqHz > lowPassCutoff) {
            shouldZero = true;
        }
        
        // Zero out the complex value completely
        if (shouldZero) {
            fftData[i][0] = 0.0; // Real part
            fftData[i][1] = 0.0; // Imaginary part
        }
    }
}

void FrequencyFilter::reset() {
    std::fill(lowPassDelayLine.begin(), lowPassDelayLine.end(), 0.0f);
    std::fill(highPassDelayLine.begin(), highPassDelayLine.end(), 0.0f);
    std::fill(bandStopDelayLine.begin(), bandStopDelayLine.end(), 0.0f);
    std::fill(bandPassDelayLine.begin(), bandPassDelayLine.end(), 0.0f);
}

bool FrequencyFilter::isActive() const {
    return lowPassEnabled || highPassEnabled || bandStopEnabled || bandPassEnabled;
}

void FrequencyFilter::generateLowPassCoeffs(float cutoffHz, float sampleRate, int filterLength) {
    lowPassCoeffs.resize(filterLength);
    float nyquist = sampleRate / 2.0f;
    float normalizedCutoff = cutoffHz / nyquist;
    
    int center = filterLength / 2;
    
    for (int i = 0; i < filterLength; i++) {
        int n = i - center;
        if (n == 0) {
            lowPassCoeffs[i] = 2.0f * normalizedCutoff;
        } else {
            lowPassCoeffs[i] = 2.0f * normalizedCutoff * sinc(2.0f * normalizedCutoff * n);
        }
        lowPassCoeffs[i] *= blackmanWindow(i, filterLength);
    }
    
    // Normalize coefficients
    float sum = 0.0f;
    for (float coeff : lowPassCoeffs) {
        sum += coeff;
    }
    if (sum > 0.0f) {
        for (float& coeff : lowPassCoeffs) {
            coeff /= sum;
        }
    }
    
    // Resize delay line to match
    lowPassDelayLine.resize(filterLength, 0.0f);
}

void FrequencyFilter::generateHighPassCoeffs(float cutoffHz, float sampleRate, int filterLength) {
    highPassCoeffs.resize(filterLength);
    float nyquist = sampleRate / 2.0f;
    float normalizedCutoff = cutoffHz / nyquist;
    
    int center = filterLength / 2;
    
    for (int i = 0; i < filterLength; i++) {
        int n = i - center;
        if (n == 0) {
            highPassCoeffs[i] = 1.0f - 2.0f * normalizedCutoff;
        } else {
            highPassCoeffs[i] = -2.0f * normalizedCutoff * sinc(2.0f * normalizedCutoff * n);
        }
        highPassCoeffs[i] *= blackmanWindow(i, filterLength);
    }
    
    // Normalize coefficients
    float sum = 0.0f;
    for (float coeff : highPassCoeffs) {
        sum += coeff;
    }
    if (sum > 0.0f) {
        for (float& coeff : highPassCoeffs) {
            coeff /= sum;
        }
    }
    
    // Resize delay line to match
    highPassDelayLine.resize(filterLength, 0.0f);
}

void FrequencyFilter::generateBandStopCoeffs(float lowHz, float highHz, float sampleRate, int filterLength) {
    // Band-stop = low-pass (below low) + high-pass (above high)
    // We'll generate it as: all-pass - band-pass
    bandStopCoeffs.resize(filterLength);
    float nyquist = sampleRate / 2.0f;
    float normalizedLow = lowHz / nyquist;
    float normalizedHigh = highHz / nyquist;
    
    int center = filterLength / 2;
    
    for (int i = 0; i < filterLength; i++) {
        int n = i - center;
        if (n == 0) {
            bandStopCoeffs[i] = 1.0f - 2.0f * (normalizedHigh - normalizedLow);
        } else {
            float lowTerm = 2.0f * normalizedLow * sinc(2.0f * normalizedLow * n);
            float highTerm = 2.0f * normalizedHigh * sinc(2.0f * normalizedHigh * n);
            bandStopCoeffs[i] = -highTerm + lowTerm;
        }
        bandStopCoeffs[i] *= blackmanWindow(i, filterLength);
    }
    
    // Normalize coefficients
    float sum = 0.0f;
    for (float coeff : bandStopCoeffs) {
        sum += coeff;
    }
    if (sum > 0.0f) {
        for (float& coeff : bandStopCoeffs) {
            coeff /= sum;
        }
    }
    
    // Resize delay line to match
    bandStopDelayLine.resize(filterLength, 0.0f);
}

void FrequencyFilter::generateBandPassCoeffs(float lowHz, float highHz, float sampleRate, int filterLength) {
    // Band-pass = high-pass (low cutoff) - high-pass (high cutoff)
    bandPassCoeffs.resize(filterLength);
    float nyquist = sampleRate / 2.0f;
    float normalizedLow = lowHz / nyquist;
    float normalizedHigh = highHz / nyquist;
    
    int center = filterLength / 2;
    
    for (int i = 0; i < filterLength; i++) {
        int n = i - center;
        if (n == 0) {
            bandPassCoeffs[i] = 2.0f * (normalizedHigh - normalizedLow);
        } else {
            float lowTerm = 2.0f * normalizedLow * sinc(2.0f * normalizedLow * n);
            float highTerm = 2.0f * normalizedHigh * sinc(2.0f * normalizedHigh * n);
            bandPassCoeffs[i] = highTerm - lowTerm;
        }
        bandPassCoeffs[i] *= blackmanWindow(i, filterLength);
    }
    
    // Normalize coefficients
    float sum = 0.0f;
    for (float coeff : bandPassCoeffs) {
        sum += coeff;
    }
    if (sum > 0.0f) {
        for (float& coeff : bandPassCoeffs) {
            coeff /= sum;
        }
    }
    
    // Resize delay line to match
    bandPassDelayLine.resize(filterLength, 0.0f);
}

float FrequencyFilter::blackmanWindow(int n, int N) {
    float a0 = 0.42f;
    float a1 = 0.5f;
    float a2 = 0.08f;
    float pi = 3.14159265358979323846f;
    return a0 - a1 * std::cos(2.0f * pi * n / (N - 1)) + a2 * std::cos(4.0f * pi * n / (N - 1));
}

float FrequencyFilter::sinc(float x) {
    if (std::abs(x) < 1e-6f) {
        return 1.0f;
    }
    float pi = 3.14159265358979323846f;
    return std::sin(pi * x) / (pi * x);
}

float FrequencyFilter::applyFIR(const std::vector<float>& coeffs, std::vector<float>& delayLine, float sample) {
    if (coeffs.empty() || delayLine.size() != coeffs.size()) {
        return sample;
    }
    
    // Shift delay line
    for (size_t i = delayLine.size() - 1; i > 0; i--) {
        delayLine[i] = delayLine[i - 1];
    }
    delayLine[0] = sample;
    
    // Compute output
    float output = 0.0f;
    for (size_t i = 0; i < coeffs.size(); i++) {
        output += coeffs[i] * delayLine[i];
    }
    
    return output;
}

float FrequencyFilter::binToHz(int bin, int fftSize, float sampleRate) {
    return (bin * sampleRate) / fftSize;
}

