SRC =  trace.c file_util.c lz4.c md5.c aes.c pro_time.c log.c test.c

test: $(SRC)
	gcc -O2 -g -Wall -o $@ $^ -lpthread
	
.PHONY: clean
clean:
	-rm test
