PROJECT_NAME = echo

TARGET_NAME = $(PROJECT_NAME)
TARGET_PATH = .
TARGET_PATH_NAME = $(TARGET_PATH)/$(TARGET_NAME)

ISE_PATH = ../../../..
ISE_INC_PATH = $(ISE_PATH)/include
ISE_LIB_PATH = $(ISE_PATH)/lib/linux
ISE_LIB_PATH_NAME = $(ISE_LIB_PATH)/libise.a

BOOST_INC_PATH = ../../../../../boost

SRC_PATH = ../../../projects/$(PROJECT_NAME)

CC = g++
CDEFINES = 
CFLAGS = -g $(CDEFINES)

SRCS = $(wildcard $(SRC_PATH)/*.cpp)

OBJS = $(SRCS:.cpp=.o)

STATIC_LIBS = $(ISE_LIB_PATH_NAME)

LIBS = -lpthread

INCS = -I$(ISE_INC_PATH) -I$(BOOST_INC_PATH)

all: $(TARGET_PATH_NAME)

$(TARGET_PATH_NAME): $(OBJS) build_libise
	@set -e;
	$(CC) -o $(TARGET_PATH_NAME) $(OBJS) $(STATIC_LIBS) $(LIBS);
	@echo "Done: $(TARGET_NAME).";

build_libise:
	@set -e; \
	make --no-print-directory -C $(ISE_PATH)/build/linux;

%.o: %.cpp
	@set -e; \
	echo "Compiling $<..."; \
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@;

clean:
	@set -e; \
	rm -f $(OBJS) $(TARGET_PATH_NAME); \
	echo "Clean up $(TARGET_NAME).";
