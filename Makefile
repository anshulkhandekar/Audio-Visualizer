CXX = g++

# Detect Qt installation path and find moc
QT_BASE := $(shell brew --prefix qtbase 2>/dev/null || echo /opt/homebrew/opt/qtbase)
# Try to find moc in common locations
MOC := $(shell find /opt/homebrew/Cellar/qtbase -name moc -type f 2>/dev/null | head -1)
ifeq ($(MOC),)
    MOC := /opt/homebrew/Cellar/qtbase/6.9.3_1/share/qt/libexec/moc
endif

# Qt uses frameworks on macOS
QT_BASE := $(shell brew --prefix qtbase 2>/dev/null || echo /opt/homebrew/opt/qtbase)
QT_CHARTS := $(shell brew --prefix qtcharts 2>/dev/null || echo /opt/homebrew/opt/qtcharts)
QT_FRAMEWORKS = $(QT_BASE)/lib
QT_CHARTS_FRAMEWORKS = $(QT_CHARTS)/lib

CXXFLAGS = -Wall -Wextra -O2 -std=c++17 \
	-I/opt/homebrew/opt/fftw/include \
	-I/opt/homebrew/opt/mad/include \
	-I/opt/homebrew/opt/portaudio/include \
	-F$(QT_FRAMEWORKS) -F$(QT_CHARTS_FRAMEWORKS) \
	-I$(QT_FRAMEWORKS)/QtCore.framework/Headers \
	-I$(QT_FRAMEWORKS)/QtWidgets.framework/Headers \
	-I$(QT_FRAMEWORKS)/QtGui.framework/Headers \
	-I$(QT_CHARTS_FRAMEWORKS)/QtCharts.framework/Headers \
	-fPIC

LDFLAGS = -L/opt/homebrew/opt/fftw/lib \
	-L/opt/homebrew/opt/mad/lib \
	-L/opt/homebrew/opt/portaudio/lib \
	-lmad -lfftw3 -lportaudio -lm \
	-framework CoreAudio -framework AudioToolbox -framework CoreFoundation \
	-F$(QT_FRAMEWORKS) -F$(QT_CHARTS_FRAMEWORKS) \
	-framework QtCore -framework QtWidgets -framework QtGui -framework QtCharts

TARGET = audio_visualizer

# Source files
SOURCES = main.cpp AudioDecoder.cpp FFTAnalyzer.cpp AudioPlayer.cpp MainWindow.cpp
HEADERS = AudioDecoder.h FFTAnalyzer.h AudioPlayer.h MainWindow.h

# MOC generated files
MOC_SOURCES = MainWindow.moc.cpp AudioPlayer.moc.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o) $(MOC_SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# MOC rules for Qt classes with Q_OBJECT
MainWindow.moc.cpp: MainWindow.h
	$(MOC) -o $@ $<

AudioPlayer.moc.cpp: AudioPlayer.h
	$(MOC) -o $@ $<

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(MOC_SOURCES) output.txt

.PHONY: all clean

