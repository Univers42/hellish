NAME = test_runner

CC = cc
CFLAGS = -Wall -Wextra -Werror

LEXER_DIR = ./srcs/lexer

CFLAGS += -I./incs/

SRCS = $(LEXER_DIR)/dyn_str.c \
       $(LEXER_DIR)/vector.c \
       $(LEXER_DIR)/lexer.c \
       $(LEXER_DIR)/test_main.c

OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS)

$(LEXER_DIR)/%.o: $(LEXER_DIR)/%.c ./incs/lexer.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
