SRC =  trace.c file_util.c lz4.c md5.c aes.c timer.c log.c test.c

test: $(SRC)
	gcc -O0 -g -Wall -o $@ $^ -lpthread
	
.PHONY: clean
clean:
	-rm test
