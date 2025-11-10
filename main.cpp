#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <mad.h>
#include <iostream>
#include <fftw3.h> 
#include <cmath>
#include <cstring>

#define FFT_SIZE 1024

#define MAD_F_FULL_24BIT 0x007fffff

int main(int argc, char **argv) {
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [filename.mp3]\n", argv[0]);
        return 255;
    }

    FILE *outfile = fopen("output.txt", "w");
    if (!outfile) {
        perror("Failed to open output.txt");
        return 1;
    }

    //Prepares stream to receive mp3 data
    //Holds data like read pointer, buffer length and byte stream 
    struct mad_stream mad_stream;
    struct mad_frame mad_frame;
    struct mad_synth mad_synth;
    
    mad_stream_init(&mad_stream);
    //Initializes synthesis structure
    //Converts into pcm data
    mad_synth_init(&mad_synth);
    //Holds data for a single .mp3 frame
    mad_frame_init(&mad_frame);

    //Pointer for output array, fftw_complex is a complex number which is the output of the fft
    fftw_complex *fftw_out;
    //Creates fft plan, basically parameters in the fft to make it more efficient
    fftw_plan fftw_plan;
    //Pointer for input array, takes in PCM data
    double *fftw_in;
    int sample_count = 0;

    //fftw_out is technically "smaller" than fftw_in as performing a fft on real data gives redundant output data due to symmetry and complex conjugates
    fftw_in = (double*) fftw_malloc(sizeof(double) * FFT_SIZE);
    fftw_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (FFT_SIZE / 2 + 1));

    //Takes in size, input and output arrays and fft "mode"
    //FFTW_ESTIMATE means that setup takes less time but takes more time for calculation
    fftw_plan = fftw_plan_dft_r2c_1d(FFT_SIZE, fftw_in, fftw_out, FFTW_ESTIMATE);

    char *filename = argv[1];

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Failed to open input file");
        fclose(outfile);
        return 1;
    }
    
    int fd = fileno(fp);

    //Gets metadata from file
    //Specifically for file size as libmad needs this for decoding .mp3 files
    struct stat metadata;
    if (fstat(fd, &metadata) < 0) {
        printf("Failed to stat %s\n", filename);
        fclose(fp);
        fclose(outfile);
        return 254;
    }
    printf("File size %ld bytes\n", (long)metadata.st_size);

    //loads .mp3 into memory
    //Input parameters basically give memory address (0 means system chooses), size, read/write modify access, file descriptor and offset
    const unsigned char* input_stream = (const unsigned char*) mmap(0, metadata.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (input_stream == MAP_FAILED) {
        perror("Failed to mmap file");
        fclose(fp);
        fclose(outfile);
        return 1;
    }

    //Initializes libmad decoder
    mad_stream_buffer(&mad_stream, input_stream, metadata.st_size);

    //Loop runs until end of .mp3 buffer is reached and break is called
    while (1) {
        //mad_frame_decode takes one frame from stream and puts it into &mad_frame
        if (mad_frame_decode(&mad_frame, &mad_stream)) {
            if (MAD_RECOVERABLE(mad_stream.error)) {
                continue;
            } else if (mad_stream.error == MAD_ERROR_BUFLEN) {
                break;
            } else {
                break;
            }
        }
        //takes decoded frame and synthesizes it into pcm data
        mad_synth_frame(&mad_synth, &mad_frame);
        unsigned int nsamples = mad_synth.pcm.length;

        //loops over every synthesized pcm data in frame sample data
        for(unsigned int i = 0; i < nsamples; i++){
            //extracts pcm data from array and normalizes it to [-1, 1]
            mad_fixed_t pcm_sample = mad_synth.pcm.samples[0][i];
            float normalized_sample = (float)(pcm_sample / (float)MAD_F_FULL_24BIT);
            
            //pcm data is copied into fft input array
            if (sample_count < FFT_SIZE) {
                fftw_in[sample_count] = normalized_sample;
                sample_count++;
            }

            //fftw is executed on fft buffer when input array is full
            if (sample_count == FFT_SIZE) {
                fftw_execute(fftw_plan); 
                printf("--- New FFT Frame ---\n");
                //loops over every bin in output data
                for (int k = 0; k < (FFT_SIZE / 2 + 1); k++) {
                    float real = fftw_out[k][0];
                    float imag = fftw_out[k][1];
                    //gets magnitude
                    float magnitude = sqrtf(real * real + imag * imag);
                    //bin is a single frequency in the spectrum
                    //higher k means higher frequency
                    //magnitude is how strong the frequency is 
                    fprintf(outfile ,"Bin %d: Mag = %f\n", k, magnitude); 
                }
                sample_count = 0;
            }
        }
    }

    //closes buffers and files
    munmap((void*)input_stream, metadata.st_size);
    fclose(fp);

    mad_synth_finish(&mad_synth);
    mad_frame_finish(&mad_frame);
    mad_stream_finish(&mad_stream);
    fclose(outfile);

    fftw_destroy_plan(fftw_plan);
    fftw_free(fftw_in);
    fftw_free(fftw_out);

    std::cout << "Closing everything" << std::endl;

    return EXIT_SUCCESS;
}
