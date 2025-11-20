#include "FFTAnalyzer.h"
#include <cstring>

FFTAnalyzer::FFTAnalyzer() 
    : sample_count(0), ready(false) {
    // Allocate FFTW arrays
    fftw_in = (double*) fftw_malloc(sizeof(double) * FFT_SIZE);
    fftw_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (FFT_SIZE / 2 + 1));
    ifftw_out = (double*) fftw_malloc(sizeof(double) * FFT_SIZE);
    
    // Create FFT plan (real to complex)
    fftw_plan = fftw_plan_dft_r2c_1d(FFT_SIZE, fftw_in, fftw_out, FFTW_ESTIMATE);
    
    // Create IFFT plan (complex to real)
    ifftw_plan_var = fftw_plan_dft_c2r_1d(FFT_SIZE, fftw_out, ifftw_out, FFTW_ESTIMATE);
    
    // Initialize magnitudes vector
    magnitudes.resize(FFT_SIZE / 2 + 1, 0.0f);
    
    // Initialize input buffer
    memset(fftw_in, 0, sizeof(double) * FFT_SIZE);
    memset(ifftw_out, 0, sizeof(double) * FFT_SIZE);
}

FFTAnalyzer::~FFTAnalyzer() {
    fftw_destroy_plan(fftw_plan);
    fftw_destroy_plan(ifftw_plan_var);
    fftw_free(fftw_in);
    fftw_free(fftw_out);
    fftw_free(ifftw_out);
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

void FFTAnalyzer::computeFFTFromBuffer(const float* buffer, int size) {
    // Copy buffer to FFT input (zero-pad if necessary)
    int copySize = std::min(size, FFT_SIZE);
    for (int i = 0; i < copySize; i++) {
        fftw_in[i] = (double)buffer[i];
    }
    // Zero-pad the rest
    for (int i = copySize; i < FFT_SIZE; i++) {
        fftw_in[i] = 0.0;
    }
    
    // Execute FFT
    fftw_execute(fftw_plan);
    
    // Calculate magnitudes
    for (int k = 0; k < (FFT_SIZE / 2 + 1); k++) {
        float real = (float)fftw_out[k][0];
        float imag = (float)fftw_out[k][1];
        float magnitude = sqrtf(real * real + imag * imag);
        magnitudes[k] = magnitude;
    }
    
    ready = true;
}

void FFTAnalyzer::performIFFT(std::vector<float>& output) {
    // Execute inverse FFT
    fftw_execute(ifftw_plan_var);
    
    // Normalize the output (IFFT in FFTW needs normalization)
    output.resize(FFT_SIZE);
    double scale = 1.0 / FFT_SIZE;
    for (int i = 0; i < FFT_SIZE; i++) {
        output[i] = (float)(ifftw_out[i] * scale);
    }
}

void FFTAnalyzer::reset() {
    sample_count = 0;
    ready = false;
    memset(fftw_in, 0, sizeof(double) * FFT_SIZE);
    memset(ifftw_out, 0, sizeof(double) * FFT_SIZE);
    std::fill(magnitudes.begin(), magnitudes.end(), 0.0f);
}

