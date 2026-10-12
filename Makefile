
CXX      = g++
CXXFLAGS = -Wall -std=c++17 -Iinclude
LIBS     = -lfreeglut -lopengl32 -lglu32 -lwinmm -lgdi32

TARGET  = build/hostel_planner.exe
SOURCES = src/main.cpp src/room.cpp src/furniture.cpp src/input.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

clean:
	rm -rf build

.PHONY: all clean
