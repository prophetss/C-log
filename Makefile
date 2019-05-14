######################################################
#make parameter
#debug/release
ver		= release
######################################################

CC		= gcc
DIRS = . ./log ./util ./lz4 ./sample
CFLAGS 	= -D_DEBUG -O0 -Wall -Wextra -ggdb
INCLUDE_DIR = -I. -I./log -I./util -I./lz4 -I./sample
LIB_L 	= -lpthread
TARGET	= log_sample_d

ifeq ($(ver), release)
	CFLAGS = -O2 -Wall -Wextra
	TARGET = log_sample_r
endif

FIND_FILES_CPP = $(wildcard $(dir)/*.c)
SOURCES = $(foreach dir, $(DIRS), $(FIND_FILES_CPP))

$(TARGET): $(SOURCES)
	$(CC) $(INCLUDE_DIR) $(CFLAGS) -o $@ $^ $(LIB_L)

	
.PHONY:clean
clean:
	rm log_sample_d log_sample
