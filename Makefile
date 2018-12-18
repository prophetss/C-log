SRC = cipher/aes.c cipher/md5.c compress/lz4_file.c compress/lz4.c log.h log.c sample/timer.c sample/test.c
DEBUG =	-O0 -Wall -Wextra -ggdb

all: $(SRC)
	gcc -I./compress -I./cipher -I../ -O2 -Wall -Wextra -o log_sample $^ -lpthread

debug: $(SRC)
	gcc -I./compress -I./cipher -I../ $(DEBUG) -o log_sample_d $^ -lpthread

.PHONY: clean
clean:
	rm log_sample log_sample_d -f