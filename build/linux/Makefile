TARGET_NAME = libise.a
TARGET_PATH = ../../lib/linux
TARGET_PATH_NAME = $(TARGET_PATH)/$(TARGET_NAME)

BOOST_INC_PATH = ../../../boost
MYSQL_INC_PATH = /usr/include/mysql

SRC_PATH = ../../src
INC_PATH = ../../include

CC = g++
CDEFINES = 
CFLAGS = -g -O2 -march=i486 $(CDEFINES)

INCS = -I$(INC_PATH) -I$(BOOST_INC_PATH) -I$(MYSQL_INC_PATH)

SRCS = $(wildcard $(SRC_PATH)/*.cpp)
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET_PATH_NAME)

$(TARGET_PATH_NAME): $(OBJS)
	@set -e; \
	ar r $(TARGET_PATH_NAME) $(OBJS); \
	ranlib $(TARGET_PATH_NAME); \
	echo "Done: $(TARGET_NAME).";

%.o: %.cpp
	@set -e; \
	echo "Compiling $<..."; \
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@;

clean:
	@set -e; \
	rm -f $(OBJS) $(TARGET_PATH_NAME); \
	echo "Clean up $(TARGET_NAME).";
