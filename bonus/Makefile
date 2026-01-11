# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: marvin <marvin@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/10 01:08:26 by marvin            #+#    #+#              #
#    Updated: 2026/01/10 01:08:26 by marvin           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Werror -g3 -ggdb -fsanitize=address,leak -std=gnu11
INCLUDES := -I./incs -I./vendor/libft/include -I./vendor/libft -I./vendor/libft/include/internals

# Source and object files
SRC_DIR := src
OBJ_DIR := build/obj

SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# Target
NAME := minishell

LIBFT_DIR = vendor/libft
LIBFT_A = $(LIBFT_DIR)/libft.a
LIBFTPRINTF_A = $(LIBFT_DIR)/libftprintf.a

.PHONY: all clean fclean re

all: $(NAME)

# Link the final binary
$(NAME): $(LIBFT_A) $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -L$(LIBFT_DIR) -lft -lreadline -o $@

# Compile .c -> .o and generate side-by-side .d dependency file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Include dependency files if present
-include $(DEPS)

clean:
	@rm -rf build

fclean: clean
	@rm -f $(NAME)

re: fclean all

$(LIBFT_A):
	$(MAKE) -C $(LIBFT_DIR) -j20

$(OBJ): | $(LIBFT_A) $(LIBFTPRINTF_A)