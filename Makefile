NAME		= codexion

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -g -pthread -fsanitize=thread #-fsanitize=address
RM			= rm -f

SRCS		= coders/main.c \
				coders/parser.c \
				coders/utils.c \
				coders/queue.c \
				coders/coder.c \
				coders/the_master.c \
				coders/sleep_room.c \
				coders/heap.c \
				coders/heap_compare.c \
				coders/heap_utils.c \
				coders/coder_actions.c \
				coders/monitor.c \
				coders/simulation.c \
				coders/waiter_logic.c \
				coders/calloc.c

OBJS		= $(SRCS:.c=.o)
HEADER		= coders/codexion.h

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
