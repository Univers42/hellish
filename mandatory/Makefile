# Makefile to build lexer and multiple test binaries into bin/

CC = cc
CFLAGS = -Wall -Wextra -Werror -g3
CFLAGS += -I./incs/ -I./incs/libft -I./incs/libft/include -I./incs/libft/include/internals

READLINE_LIB = -lreadline

LEXER_DIR = ./srcs/lexer
LIBFT_DIR = ./incs/libft
LIBFT = $(LIBFT_DIR)/libft.a
BIN_DIR = ./bin

# lexer sources
LEXER_SRCS = $(LEXER_DIR)/dyn_str.c \
		$(LEXER_DIR)/dyn_str_utils.c \
       $(LEXER_DIR)/vector.c \
       $(LEXER_DIR)/lexer.c \
	   $(LEXER_DIR)/lexer_handlers.c \
	   $(LEXER_DIR)/lexer_classifiers.c \
	   $(LEXER_DIR)/lexer_redirect_handler.c \
	   $(LEXER_DIR)/lexer_quote_handlers.c

LEXER_OBJS = $(LEXER_SRCS:.c=.o)

# test sources discovered automatically
TEST_SRCS = $(wildcard srcs/tests/*.c)
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_BINS = $(patsubst srcs/tests/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

all: $(TEST_BINS)

# Link each test into bin/<name>
$(BIN_DIR)/%: $(LEXER_OBJS) srcs/tests/%.o $(LIBFT)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(LEXER_OBJS) srcs/tests/$*.o $(LIBFT) $(READLINE_LIB)

# Build libft static library
$(LIBFT):
	$(MAKE) -C $(LIBFT_DIR) -j20

# Compile rules
$(LEXER_DIR)/%.o: $(LEXER_DIR)/%.c ./incs/lexer.h
	$(CC) $(CFLAGS) -c $< -o $@

srcs/%.o: srcs/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LEXER_OBJS) $(TEST_OBJS)
	$(MAKE) -C $(LIBFT_DIR) clean

fclean: clean
	rm -rf $(BIN_DIR)
	$(MAKE) -C $(LIBFT_DIR) fclean

re: fclean all

.PHONY: all clean fclean re
