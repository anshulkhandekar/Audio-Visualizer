# Audio-Visualizer
CSCE 120 Project (Building an audio visualizer and real-time audio editor in C++)

## Description
This program decodes MP3 audio files into PCM data, performs FFT analysis using FFTW3, and outputs frequency magnitude data to a text file.

(Milestone 1 Done) The program will:
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

(Milestone 2 Done) The program will:

The GUI will open with:
1. Load File button - Opens a file dialog to select MP3 or WAV files
2. Play button - Starts playback and visualization
3. Pause button - Pauses/resumes playback
4. Stop button - Stops playback and resets visualization
5. Frequency Spectrum Chart - Real-time bar chart showing frequency magnitudes

