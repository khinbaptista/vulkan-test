##################################################
#
#	Makefile template
#		by Khin Baptista
#
##################################################

CC	= g++
LINKER	= g++
CGLSL	= glslangValidator

CFLAGS	= -Wall -std=c++14
LDFLAGS	= -lvulkan
GLFLAGS = -V

DEBUG = 1
DEBUG_FLAGS = -g -ggdb

##################################################

# Name of the project (executable binary)
Project	= vulkan

# List of packages to use with 'pkg-config'
Packages = glfw3

# Path to directories
SourcePath  = source
ObjectsPath = source/objects

# Path to place compiled shaders
ShadersPath = shaders

# Source files names
SourceFiles = main.cpp window.cpp application.cpp

# Shader source files (GLSL)
ShaderFiles = vertex.vert fragment.frag

##################################################

CPP = $(patsubst %, $(SourcePath)/%, $(SourceFiles))
OBJ = $(patsubst $(SourcePath)/%.cpp, $(ObjectsPath)/%.o, $(CPP))
DEP = $(patsubst %.o, %.d, $(OBJ))

GLSL = $(patsubst %, $(SourcePath)/%, $(ShaderFiles))
VERT = $(filter %.vert, $(GLSL))
FRAG = $(filter %.frag, $(GLSL))

SPIRV  = $(patsubst $(SourcePath)/%.vert, $(ShadersPath)/%-v.spv, $(VERT))
SPIRV += $(patsubst $(SourcePath)/%.frag, $(ShadersPath)/%-f.spv, $(FRAG))

CFLAGS +=  `pkg-config --cflags $(Packages)`
LDFLAGS += `pkg-config --static --libs $(Packages)`

ifeq ($(DEBUG), 0)
CFLAGS += -DNDEBUG
else
CFLAGS += $(DEBUG_FLAGS)
endif

##################################################

.PHONY: all clean

all: objectdir shaders $(Project)

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
	rm -f $(ObjectsPath)/*.* $(Project) *.spv
	rmdir $(ObjectsPath)

##################################################

shaders: $(SPIRV)

$(ShadersPath)/%-f.spv: $(SourcePath)/%.frag
	$(CGLSL) $(GLFLAGS) -o $@ $^

$(ShadersPath)/%-v.spv: $(SourcePath)/%.vert
	$(CGLSL) $(GLFLAGS) -o $@ $^

##################################################
