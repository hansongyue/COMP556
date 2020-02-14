.SUFFIXES:.c .o

CC=clang

SRCS1=server.c
OBJS1=$(SRCS1:.c=.o)
EXEC1=server

SRCS2=client.c
OBJS2=$(SRCS2:.c=.o)
EXEC2=client

start: $(OBJS1) $(OBJS2)
	$(CC) -o $(EXEC1) $(OBJS1)
	$(CC) -o $(EXEC2) $(OBJS2)
	@echo "-----------------------------OK-----------------------"

.c.o:
	$(CC) -Wall -g -o $@ -c $<

clean:
	rm -rf $(EXEC1) $(OBJS1)
	rm -rf $(EXEC2) $(OBJS2)
