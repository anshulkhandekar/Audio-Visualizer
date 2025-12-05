#include "AudioDecoder.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>

AudioDecoder::AudioDecoder() 
    : sample_rate(0), channels(0), loaded(false),
      input_stream(nullptr), file_size(0), file_descriptor(-1) {
    mad_stream_init(&mad_stream);
    mad_synth_init(&mad_synth);
    mad_frame_init(&mad_frame);
}

AudioDecoder::~AudioDecoder() {
    cleanup();
    mad_synth_finish(&mad_synth);
    mad_frame_finish(&mad_frame);
    mad_stream_finish(&mad_stream);
}

bool AudioDecoder::loadFile(const std::string& filename) {
    clear();
    return decodeMP3(filename);
}

bool AudioDecoder::decodeMP3(const std::string& filename) {
    // Open file
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    
    file_descriptor = fileno(fp);
    
    // Get file metadata
    struct stat metadata;
    if (fstat(file_descriptor, &metadata) < 0) {
        std::cerr << "Failed to stat file: " << filename << std::endl;
        fclose(fp);
        return false;
    }
    
    file_size = metadata.st_size;
    std::cout << "File size: " << file_size << " bytes" << std::endl;
    
    // Memory map the file
    input_stream = (const unsigned char*) mmap(0, file_size, PROT_READ, MAP_SHARED, file_descriptor, 0);
    if (input_stream == MAP_FAILED) {
        std::cerr << "Failed to mmap file" << std::endl;
        fclose(fp);
        file_descriptor = -1;
        return false;
    }
    
    // Don't close fp yet - we need the file descriptor for munmap later
    // Store it but don't close until cleanup
    
    // Initialize libmad decoder
    mad_stream_buffer(&mad_stream, input_stream, file_size);
    
    // Decode all frames
    while (1) {
        if (mad_frame_decode(&mad_frame, &mad_stream)) {
            if (MAD_RECOVERABLE(mad_stream.error)) {
                continue;
            } else if (mad_stream.error == MAD_ERROR_BUFLEN) {
                break;
            } else {
                break;
            }
        }
        
        // Get sample rate from first frame
        if (sample_rate == 0) {
            sample_rate = mad_frame.header.samplerate;
            channels = MAD_NCHANNELS(&mad_frame.header);
            std::cout << "Sample rate: " << sample_rate << " Hz" << std::endl;
            std::cout << "Channels: " << channels << std::endl;
        }
        
        // Synthesize frame to PCM
        mad_synth_frame(&mad_synth, &mad_frame);
        unsigned int nsamples = mad_synth.pcm.length;
        
        // Extract and normalize PCM samples
        for (unsigned int i = 0; i < nsamples; i++) {
            // Use left channel (or mono)
            mad_fixed_t pcm_sample = mad_synth.pcm.samples[0][i];
            // libmad uses fixed-point format: MAD_F_ONE (0x10000000) = 1.0
            // Convert to float in range [-1.0, 1.0]
            float normalized_sample = (float)mad_f_todouble(pcm_sample);
            samples.push_back(normalized_sample);
        }
    }
    
    // Close the FILE* but keep file_descriptor for cleanup
    fclose(fp);
    // Note: file_descriptor is still valid for munmap in cleanup()
    loaded = true;
    std::cout << "Decoded " << samples.size() << " samples" << std::endl;
    
    return true;
}

void AudioDecoder::cleanup() {
    if (input_stream != nullptr && input_stream != MAP_FAILED) {
        munmap((void*)input_stream, file_size);
        input_stream = nullptr;
    }
    // Note: file_descriptor was from a FILE* that we already closed with fclose()
    // We don't need to close it again, but we reset it
    file_descriptor = -1;
}

void AudioDecoder::clear() {
    samples.clear();
    sample_rate = 0;
    channels = 0;
    loaded = false;
    cleanup();
}

