# Audio-Visualizer
CSCE 120 Project (Building an audio visualizer and real-time audio editor in C++)

## Description
This program decodes MP3 audio files into PCM data, performs FFT analysis using FFTW3, and outputs frequency magnitude data to a text file.

## Dependencies

### macOS (using Homebrew)
```bash
# Install FFTW3
brew install fftw

# Install libmad (MP3 decoder)
# Note: libmad may require manual installation or alternative
brew install mad
# If the above doesn't work, you may need to install from source or use an alternative library
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install libmad0-dev libfftw3-dev build-essential
```

## Building
```bash
make
```

## Usage
```bash
./audio_visualizer <input.mp3>
```

The program will:
1. Decode the MP3 file into PCM samples
2. Process samples in chunks of 1024 (FFT_SIZE)
3. Perform FFT analysis on each chunk
4. Calculate frequency magnitudes
5. Write results to `output.txt`

## Output Format
The `output.txt` file contains frequency bin data in the format:
```
Bin 0: Mag = <magnitude>
Bin 1: Mag = <magnitude>
...
```

Each bin represents a frequency component, with higher bin numbers representing higher frequencies.
