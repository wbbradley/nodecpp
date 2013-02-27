UNAME := $(shell uname)
DEBUG_FLAGS := -DDEBUG -g -O0
NDEBUG_FLAGS := -g -O3

ifeq ($(UNAME),Darwin)
	CPP = clang -std=gnu++0x -DMACOS
	CC = clang -DMACOS
	LINKER = clang -stdlib=libstdc++ -lstdc++ -L../libuv -framework Cocoa
	LINKER_OPTS := $(NDEBUG_FLAGS)
	LINKER_DEBUG_OPTS := $(DEBUG_FLAGS)
else
	CPP = g++ -std=c++0x
	CC = gcc
	LINKER = g++ -stdlib=libc++0x
	LINKER_OPTS := -pthread $(NDEBUG_FLAGS)
	LINKER_DEBUG_OPTS := -pthread $(DEBUG_FLAGS)
endif

BUILD_DIR = build

CFLAGS := \
	-I../libuv/include \
	-c \
	-Wall \
	-pthread \
	-DDEBUG \
	-g \

JUMBLE_SOURCES = \
				 cmd_options.cpp \
				 disk.cpp \
				 nodecpp.cpp \
				 logger.cpp \
				 utils.cpp \

JUMBLE_OBJECTS = $(addprefix $(BUILD_DIR)/,$(JUMBLE_SOURCES:.cpp=.o))
JUMBLE_TARGET = nodecpp

TARGETS = $(JUMBLE_TARGET)

all: build-dir $(TARGETS)

build-dir:
	@if test ! -d $(BUILD_DIR); then mkdir -p $(BUILD_DIR); fi

$(JUMBLE_TARGET): $(JUMBLE_OBJECTS)
	$(LINKER) $(LINKER_OPTS) -luv $(JUMBLE_OBJECTS) -o $(JUMBLE_TARGET)

$(BUILD_DIR)/%.o: %.cpp
	$(CPP) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

CLEAN = rm -rf $(BUILD_DIR)/*.o $(TARGETS)

clean:
	$(CLEAN)

