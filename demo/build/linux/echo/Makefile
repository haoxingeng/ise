PROJECT_NAME = echo

TARGET_NAME = $(PROJECT_NAME)
TARGET_PATH = .
TARGET_PATH_NAME = $(TARGET_PATH)/$(TARGET_NAME)

SSE_PATH = ../../../..
SSE_INC_PATH = $(SSE_PATH)/include
SSE_LIB_PATH = $(SSE_PATH)/lib/linux
SSE_LIB_PATH_NAME = $(SSE_LIB_PATH)/libsse.a

SRC_PATH = ../../../projects/$(PROJECT_NAME)/

CC = g++
CDEFINES = 
CFLAGS = -g $(CDEFINES)

SRCS = $(wildcard $(SRC_PATH)/*.cpp)

OBJS = $(SRCS:.cpp=.o)

STATIC_LIBS = $(SSE_LIB_PATH_NAME)

LIBS = -lpthread

INCS = -I$(SSE_INC_PATH)

all: $(TARGET_PATH_NAME)

$(TARGET_PATH_NAME): $(OBJS) build_libsse
	@set -e;
	$(CC) -o $(TARGET_PATH_NAME) $(OBJS) $(STATIC_LIBS) $(LIBS);
	@echo "Done: $(TARGET_NAME).";

build_libsse:
	@set -e; \
	make --no-print-directory -C $(SSE_PATH)/build/linux;

%.o: %.cpp
	@set -e; \
	echo "Compiling $<..."; \
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@;

clean:
	@set -e; \
	rm -f $(OBJS) $(TARGET_PATH_NAME); \
	echo "Clean up $(TARGET_NAME).";
