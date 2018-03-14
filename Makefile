SRC = md5.c sha2.c aes.c pro_time.c log.c test.c

test: $(SRC)
	gcc -g -Wall -o $@ $^ -lpthread
	
.PHONY: clean
clean:
	-rm test