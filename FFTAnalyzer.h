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
    
    // Get the latest FFT magnitudes (size: FFT_SIZE/2 + 1)
    const std::vector<float>& getMagnitudes() const { return magnitudes; }
    
    // Reset the analyzer (clear buffer)
    void reset();
    
    // Check if FFT is ready
    bool isReady() const { return ready; }

private:
    double* fftw_in;
    fftw_complex* fftw_out;
    fftw_plan fftw_plan;
    std::vector<float> magnitudes;
    int sample_count;
    bool ready;
    
    void computeFFT();
};

#endif // FFTANALYZER_H

