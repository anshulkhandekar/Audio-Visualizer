#include "FFTAnalyzer.h"
#include <cstring>

FFTAnalyzer::FFTAnalyzer() 
    : sample_count(0), ready(false) {
    // Allocate FFTW arrays
    fftw_in = (double*) fftw_malloc(sizeof(double) * FFT_SIZE);
    fftw_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (FFT_SIZE / 2 + 1));
    
    // Create FFT plan
    fftw_plan = fftw_plan_dft_r2c_1d(FFT_SIZE, fftw_in, fftw_out, FFTW_ESTIMATE);
    
    // Initialize magnitudes vector
    magnitudes.resize(FFT_SIZE / 2 + 1, 0.0f);
    
    // Initialize input buffer
    memset(fftw_in, 0, sizeof(double) * FFT_SIZE);
}

FFTAnalyzer::~FFTAnalyzer() {
    fftw_destroy_plan(fftw_plan);
    fftw_free(fftw_in);
    fftw_free(fftw_out);
}

bool FFTAnalyzer::addSample(float sample) {
    if (sample_count < FFT_SIZE) {
        fftw_in[sample_count] = (double)sample;
        sample_count++;
        ready = false;
    }
    
    // When buffer is full, compute FFT
    if (sample_count == FFT_SIZE) {
        computeFFT();
        sample_count = 0;
        ready = true;
        return true;
    }
    
    return false;
}

void FFTAnalyzer::computeFFT() {
    // Execute FFT
    fftw_execute(fftw_plan);
    
    // Calculate magnitudes
    for (int k = 0; k < (FFT_SIZE / 2 + 1); k++) {
        float real = (float)fftw_out[k][0];
        float imag = (float)fftw_out[k][1];
        float magnitude = sqrtf(real * real + imag * imag);
        magnitudes[k] = magnitude;
    }
}

void FFTAnalyzer::reset() {
    sample_count = 0;
    ready = false;
    memset(fftw_in, 0, sizeof(double) * FFT_SIZE);
    std::fill(magnitudes.begin(), magnitudes.end(), 0.0f);
}

