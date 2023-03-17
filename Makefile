CC = gcc
CFLAGS = -Wall -Wextra -Wundef

TARGET = mysh
OBJS = mysh.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

mysh.o: mysh.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)
