# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/11/06 20:32:02 by                   #+#    #+#              #
#    Updated: 2025/11/06 20:32:27 by dlesieur         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME        := libmemalloc.a
TEST_BIN    := test_memalloc
TEST_TYPES  := test_types

CC          := cc
CFLAGS      := -Wall -Wextra -Werror -std=c99
AR          := ar
ARFLAGS     := rcs
RM          := rm -f

INCS        := -I/home/dlesieur/Documents/42_school/shell/sh42/incs

SRCS        :=	\
				dyn_arena.c \
				block.c \
				check.c \
				helper_stack.c \
				grab.c \
				st_all.c \
				stack.c \
				utils.c \
				wrapper.c \
				set.c

OBJDIR      := obj
OBJS        := $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))

TEST_SRCS   := main.c
TEST_OBJS   := $(addprefix $(OBJDIR)/, $(TEST_SRCS:.c=.o))

TEST_TYPES_SRCS := test_types.c
TEST_TYPES_OBJS := $(addprefix $(OBJDIR)/, $(TEST_TYPES_SRCS:.c=.o))

all: $(OBJDIR) $(NAME)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(NAME): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: %.c arena.h
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(OBJDIR)/main.o: main.c arena.h
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(OBJDIR)/test_types.o: test_types.c arena.h
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(TEST_BIN): $(NAME) $(TEST_OBJS)
	$(CC) $(CFLAGS) $(INCS) $(TEST_OBJS) -L. -lmemalloc -o $@

$(TEST_TYPES): $(NAME) $(TEST_TYPES_OBJS)
	$(CC) $(CFLAGS) $(INCS) $(TEST_TYPES_OBJS) -L. -lmemalloc -o $@

run_test: $(TEST_BIN)
	./$(TEST_BIN)

run_test_types: $(TEST_TYPES)
	./$(TEST_TYPES)

run_all: $(TEST_BIN) $(TEST_TYPES)
	./$(TEST_BIN)
	./$(TEST_TYPES)

valgrind_test: $(TEST_BIN)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TEST_BIN)

valgrind_test_types: $(TEST_TYPES)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TEST_TYPES)

clean:
	$(RM) $(OBJS) $(TEST_OBJS) $(TEST_TYPES_OBJS)

fclean: clean
	$(RM) $(NAME) $(TEST_BIN) $(TEST_TYPES)

re: fclean all

.PHONY: all run_test run_test_types run_all valgrind_test valgrind_test_types clean fclean re
