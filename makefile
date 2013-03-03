UNAME := $(shell uname)
DEBUG_FLAGS := -DDEBUG -g -O0
NDEBUG_FLAGS := -g -O3

ifeq ($(UNAME),Darwin)
	CPP = clang++ -std=c++0x -stdlib=libc++ -DMACOS
	CC = clang -DMACOS
	LINKER = clang++ -stdlib=libc++ -Ldeps/http_parser -Ldeps/libuv -framework Cocoa
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
	-Ideps/libuv/include \
	-Ideps/http_parser \
	-c \
	-Wall \
	-pthread \
	-DDEBUG \
	-g \

SAMPLE_SOURCES = \
				 cmd_options.cpp \
				 disk.cpp \
				 http_client.cpp \
				 http_server.cpp \
				 sample.cpp \
				 logger.cpp \
				 nodecpp_errors.cpp \
				 utils.cpp \

SAMPLE_OBJECTS = $(addprefix $(BUILD_DIR)/,$(SAMPLE_SOURCES:.cpp=.o))
SAMPLE_TARGET = nodecpp-sample

TARGETS = $(SAMPLE_TARGET)

all: build-dir $(TARGETS) README.pdf

README.pdf: README.md
	@md2pdf README.md || (touch README.pdf && rm README.pdf)

build-dir:
	@if test ! -d $(BUILD_DIR); then mkdir -p $(BUILD_DIR); fi

$(SAMPLE_TARGET): $(SAMPLE_OBJECTS) $(HTTP_PARSER_OBJECTS)
	$(LINKER) $(LINKER_OPTS) -luv deps/http_parser/http_parser_g.o $(SAMPLE_OBJECTS) -o $(SAMPLE_TARGET)

$(BUILD_DIR)/%.o: %.cpp
	$(CPP) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

CLEAN = rm -rf $(BUILD_DIR) README.pdf $(TARGETS)

clean:
	$(CLEAN)

