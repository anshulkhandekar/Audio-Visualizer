# Audio-Visualizer
CSCE 120 Project (Building an audio visualizer and real-time audio editor in C++)

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
