##################################################

##################################################

Project	= vulkan

CC	= g++
LINKER	= g++

##################################################

CFLAGS	= -Wall -std=c++14
LDFLAGS	= -lvulkan -L$(VULKAN_SDK)/lib

##################################################

SourcePath	= source
ObjectsPath	= source/objects

SourceFiles	= main.cpp vk_application.cpp

Packages	= glfw3

##################################################

CPP = $(patsubst %, $(SourcePath)/%, $(SourceFiles))
OBJ = $(patsubst $(SourcePath)/%.cpp, $(ObjectsPath)/%.o, $(CPP))

CFLAGS	+= `pkg-config --cflags $(Packages)`
#-DNDEBUG
LDFLAGS	+= `pkg-config --static --libs $(Packages)`

##################################################

.PHONY: all test prepare clean

all: object_dir $(Project)

object_dir:
	mkdir -p $(ObjectsPath)

test: $(Project)
	./$(Project)

remake: clean all

build: remake test

$(Project): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(ObjectsPath)/%.o: $(SourcePath)/%.cpp
	$(CC) -c -o $@ $^ $(CFLAGS)

##################################################

prepare:
	mkdir $(SourcePath) $(ObjectsPath)

clean:
	rm -f $(ObjectsPath)/*.o $(Project)
