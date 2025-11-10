NAME = libtermcap.a
CC = gcc
CFLAGS = -Wall -Wextra -Werror -I.
AR = ar rcs

SRCS = tgetent.c \
       tgetst1.c \
       tgetnum.c \
       tgetstr.c \
       tgetflag.c \
       helpers.c \
       gobble_line.c \
       tputs.c \
       tprint.c \
       tparam.c \
       tgoto.c \
       xmalloc.c

OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(AR) $(NAME) $(OBJS)

%.o: %.c termcap.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

test: $(NAME)
	$(CC) $(CFLAGS) test_termcap.c -L. -ltermcap -o test_termcap

.PHONY: all clean fclean re test
