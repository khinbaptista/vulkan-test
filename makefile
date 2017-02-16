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

SourceFiles	= main.cpp vk_app.cpp
#lication.cpp

Packages	= glfw3

##################################################

CPP = $(patsubst %, $(SourcePath)/%, $(SourceFiles))
OBJ = $(patsubst $(SourcePath)/%.cpp, $(ObjectsPath)/%.o, $(CPP))
DEP = $(OBJ:%.o=%.d)

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

#depend: $(DEP)

$(Project): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

#$(DEP): $(CPP)
#	rm -f $(DEP)
#	$(CC) $(CFLAGS) -MM $^ -MF $(DEP);

-include $(DEP)
$(ObjectsPath)/%.o: $(SourcePath)/%.cpp
	$(CC) -MMD -c -o $@ $< $(CFLAGS)

##################################################

prepare:
	mkdir $(SourcePath) $(ObjectsPath)

clean:
	rm -f $(ObjectsPath)/*.o $(ObjectsPath)/*.d $(Project)
