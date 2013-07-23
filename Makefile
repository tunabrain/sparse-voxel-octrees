CC = gcc
CXX = g++
RM = rm -f
WARNINGS = -Wall -Wformat -Wcast-align
CFLAGS = -Isrc/ -O2 $(WARNINGS)

ifeq ($(OS),Windows_NT)
    LDFLAGS = -lmingw32 -lSDLmain -lSDL
    TARGET = svo.exe
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        LDFLAGS = -lSDL
        TARGET = svo
    endif
endif

MATH_OBJS = Mat4.o Vec3.o Vec4.o MatrixStack.o
SVO_OBJS = Debug.o Events.o Main.o Util.o VoxelData.o VoxelOctree.o \
	PlyLoader.o tribox3.o ply/plyfile.o ThreadBarrier.o \
	$(addprefix math/,$(MATH_OBJS))
OBJECTS = $(addprefix src/,$(SVO_OBJS))

svo: $(OBJECTS)
	$(CXX) $(CFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $^
	
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	$(RM) $(TARGET)
	$(RM) -f $(OBJECTS)

.PHONY: clean svo
