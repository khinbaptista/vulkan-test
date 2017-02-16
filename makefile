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
LDFLAGS	= -lvulkan -L$(VULKAN_SDK)/lib
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

# Source files names
SourceFiles = main.cpp vk_app.cpp

# Shader source files (GLSL)
ShaderFiles = vertex.vert fragment.frag

##################################################

CPP = $(patsubst %, $(SourcePath)/%, $(SourceFiles))
OBJ = $(patsubst $(SourcePath)/%.cpp, $(ObjectsPath)/%.o, $(CPP))
DEP = $(patsubst %.o, %.d, $(OBJ))

GLSL = $(patsubst %, $(SourcePath)/%, $(ShaderFiles))
VERT = $(filter %.vert, $(GLSL))
FRAG = $(filter %.frag, $(GLSL))

SPIRV  = $(patsubst $(SourcePath)/%.vert, $(ObjectsPath)/%-v.spv, $(VERT))
SPIRV += $(patsubst $(SourcePath)/%.frag, $(ObjectsPath)/%-f.spv, $(FRAG))

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
	rm -f $(ObjectsPath)/*.* $(Project)
	rmdir $(ObjectsPath)

##################################################

shaders: $(SPIRV)

#$(ObjectsPath)/%.spv: $(SourcePath)/%
#	$(CGLSL) $(GLFLAGS) -o $@ $^

$(ObjectsPath)/%-f.spv: $(SourcePath)/%.frag
	$(CGLSL) $(GLFLAGS) -o $@ $^

$(ObjectsPath)/%-v.spv: $(SourcePath)/%.vert
	$(CGLSL) $(GLFLAGS) -o $@ $^

##################################################
