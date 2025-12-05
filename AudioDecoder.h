#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <vector>
#include <string>
#include <mad.h>

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();
    
    // Load and decode an audio file (MP3)
    bool loadFile(const std::string& filename);
    
    // Get decoded PCM samples (normalized to [-1, 1])
    const std::vector<float>& getSamples() const { return samples; }
    
    // Get sample rate
    unsigned int getSampleRate() const { return sample_rate; }
    
    // Get number of channels
    unsigned int getChannels() const { return channels; }
    
    // Check if file is loaded
    bool isLoaded() const { return loaded; }
    
    // Clear loaded data
    void clear();

private:
    std::vector<float> samples;
    unsigned int sample_rate;
    unsigned int channels;
    bool loaded;
    
    // libmad structures
    struct mad_stream mad_stream;
    struct mad_frame mad_frame;
    struct mad_synth mad_synth;
    
    // File mapping
    const unsigned char* input_stream;
    size_t file_size;
    int file_descriptor;
    
    bool decodeMP3(const std::string& filename);
    void cleanup();
};

#endif // AUDIODECODER_H

