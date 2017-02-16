##################################################
#
#	Makefile template
#		by Khin Baptista
#
##################################################

CC	= g++
LINKER	= g++

CFLAGS	= -Wall -std=c++14
LDFLAGS	= -lvulkan -L$(VULKAN_SDK)/lib

DEBUG = 1

##################################################

# Name of the project (executable binary)
Project	= vulkan

# List of packages to use with 'pkg-config'
Packages = glfw3

# Path to directories
SourcePath  = source
ObjectsPath = source/objects

# Source files names
SourceFiles = main.cpp vk_app.cpp

##################################################

CPP = $(patsubst %, $(SourcePath)/%, $(SourceFiles))
OBJ = $(patsubst $(SourcePath)/%.cpp, $(ObjectsPath)/%.o, $(CPP))
DEP = $(OBJ:%.o=%.d)

CFLAGS +=  `pkg-config --cflags $(Packages)`
LDFLAGS += `pkg-config --static --libs $(Packages)`

ifeq ($(DEBUG), 1)
CFLAGS += -DNDEBUG
endif

##################################################

.PHONY: all clean

all: objectdir $(Project)

objectdir:
	mkdir -p $(ObjectsPath)

test: all
	./$(Project)

remake: clean all

$(Project): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

-include $(DEP)
$(ObjectsPath)/%.o: $(SourcePath)/%.cpp
	$(CC) -MMD -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(ObjectsPath)/*.o $(ObjectsPath)/*.d $(Project)
	rmdir $(ObjectsPath)

##################################################
