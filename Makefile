CC = gcc
CFLAGS = -std=c99 -Wall -g -fsanitize=address,undefined 

TARGET = mysh
OBJS = mysh.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

mysh.o: mysh.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)
