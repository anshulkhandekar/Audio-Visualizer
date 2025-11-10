CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++11 -I/opt/homebrew/opt/fftw/include -I/opt/homebrew/opt/mad/include
LDFLAGS = -L/opt/homebrew/opt/fftw/lib -L/opt/homebrew/opt/mad/lib -lmad -lfftw3 -lm

TARGET = audio_visualizer
SOURCE = main.cpp

# Default target
all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)

clean:
	rm -f $(TARGET) output.txt

.PHONY: all clean

