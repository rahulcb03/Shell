CC = gcc
CFLAGS = -std=c99 -Wall -g -fsanitize=address,undefined 

mysh: mysh.c
	$(CC) $(CFLAGS) -c mymalloc.c
	$(CC) $(CFLAGS) -o mysh mysh.c

clean:
	
