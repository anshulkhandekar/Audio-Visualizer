#ifndef AUDIOEXPORTER_H
#define AUDIOEXPORTER_H

#include <string>
#include <vector>

class AudioExporter {
public:
    // Export floating-point PCM samples in [-1, 1] to a 16-bit PCM WAV file.
    // Channels controls how many channels to write; if channels > 1 and the
    // samples are mono, they will be duplicated across channels.
    static bool exportToWav(const std::string& path,
                            const std::vector<float>& samples,
                            unsigned int sampleRate,
                            unsigned int channels);
};

#endif // AUDIOEXPORTER_H


