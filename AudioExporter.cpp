#include "AudioExporter.h"

#include <cstdint>
#include <fstream>
#include <algorithm>

namespace {

inline void writeLE16(std::ofstream& out, std::uint16_t value) {
    char bytes[2];
    bytes[0] = static_cast<char>(value & 0xFF);
    bytes[1] = static_cast<char>((value >> 8) & 0xFF);
    out.write(bytes, 2);
}

inline void writeLE32(std::ofstream& out, std::uint32_t value) {
    char bytes[4];
    bytes[0] = static_cast<char>(value & 0xFF);
    bytes[1] = static_cast<char>((value >> 8) & 0xFF);
    bytes[2] = static_cast<char>((value >> 16) & 0xFF);
    bytes[3] = static_cast<char>((value >> 24) & 0xFF);
    out.write(bytes, 4);
}

} // namespace

bool AudioExporter::exportToWav(const std::string& path,
                                const std::vector<float>& samples,
                                unsigned int sampleRate,
                                unsigned int channels) {
    if (samples.empty() || sampleRate == 0 || channels == 0) {
        return false;
    }

    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    // We treat input as mono PCM; if channels > 1 we duplicate samples.
    std::uint32_t numInputSamples = static_cast<std::uint32_t>(samples.size());
    std::uint32_t totalSamples = numInputSamples * channels;
    std::uint32_t bytesPerSample = sizeof(std::int16_t);
    std::uint32_t dataChunkSize = totalSamples * bytesPerSample;
    std::uint32_t riffChunkSize = 36 + dataChunkSize;

    // ---- RIFF header ----
    out.write("RIFF", 4);
    writeLE32(out, riffChunkSize);
    out.write("WAVE", 4);

    // ---- fmt chunk ----
    out.write("fmt ", 4);
    writeLE32(out, 16);                      // Subchunk1Size (PCM)
    writeLE16(out, 1);                       // AudioFormat = 1 (PCM)
    writeLE16(out, static_cast<std::uint16_t>(channels)); // NumChannels
    writeLE32(out, sampleRate);              // SampleRate
    std::uint32_t byteRate = sampleRate * channels * bytesPerSample;
    writeLE32(out, byteRate);                // ByteRate
    std::uint16_t blockAlign = static_cast<std::uint16_t>(channels * bytesPerSample);
    writeLE16(out, blockAlign);              // BlockAlign
    writeLE16(out, 16);                      // BitsPerSample

    // ---- data chunk ----
    out.write("data", 4);
    writeLE32(out, dataChunkSize);

    // Write samples
    for (std::uint32_t i = 0; i < numInputSamples; ++i) {
        // Clamp float sample to [-1, 1] and convert to int16
        float s = std::max(-1.0f, std::min(1.0f, samples[i]));
        std::int16_t value = static_cast<std::int16_t>(s * 32767.0f);

        for (unsigned int ch = 0; ch < channels; ++ch) {
            writeLE16(out, static_cast<std::uint16_t>(value));
        }
    }

    out.close();
    return true;
}


