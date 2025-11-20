#ifndef FFTANALYZER_H
#define FFTANALYZER_H

#include <fftw3.h>
#include <vector>
#include <cmath>

#define FFT_SIZE 1024

class FFTAnalyzer {
public:
    FFTAnalyzer();
    ~FFTAnalyzer();
    
    // Process a single sample (returns true when FFT is ready)
    bool addSample(float sample);
    
    // Compute FFT directly from a buffer (for audio filtering)
    void computeFFTFromBuffer(const float* buffer, int size);
    
    // Get the latest FFT magnitudes (size: FFT_SIZE/2 + 1)
    const std::vector<float>& getMagnitudes() const { return magnitudes; }
    
    // Get the complex FFT output (for filtering and IFFT)
    fftw_complex* getFFTOutput() { return fftw_out; }
    const fftw_complex* getFFTOutput() const { return fftw_out; }
    
    // Perform inverse FFT on modified complex data and get time-domain samples
    void performIFFT(std::vector<float>& output);
    
    // Reset the analyzer (clear buffer)
    void reset();
    
    // Check if FFT is ready
    bool isReady() const { return ready; }

private:
    double* fftw_in;
    fftw_complex* fftw_out;
    double* ifftw_out; // Output buffer for IFFT
    fftw_plan ifftw_plan_var; // Inverse FFT plan  
    fftw_plan fftw_plan;
    std::vector<float> magnitudes;
    int sample_count;
    bool ready;
    
    void computeFFT();
};

#endif // FFTANALYZER_H

