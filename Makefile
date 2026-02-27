NAME		= codexion

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -pthread
RM			= rm -f

SRCS		= main.c \
				parse.c \
				init.c \
				routine.c \
				arbiter.c \
				utils.c

OBJS		= $(SRCS:.c=.o)
HEADER		= codexion.h

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
