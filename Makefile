
NAME=echo_srv
OBJS=src/echo_srv.o
CFLAGS=-Wall -Wextra -Werror -lev -I. -g -O0 -std=gnu99

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

echo_srv.o: src/echo_srv.c

clean:
	rm -rf src/*.o
	rm -f $(NAME)

PHONY: run
