# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/12 01:49:32 by dlesieur          #+#    #+#              #
#    Updated: 2026/01/12 01:49:32 by dlesieur         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Compiler and flags
CC := gcc
CFLAGS		:= -Wall -Wextra -Werror -std=gnu99	-D_XOPEN_SOURCE=700
OPTFLAGS	:= -O2 -march=native
LDFLAGS		:= -flto
DEBFLAGS	:= -g3 -ggdb -fsanitize=address,leak

# Choose flags: default = debug; pass OPT when calling make to enable optimizations
ifdef OPT
ALLFLAGS := $(CFLAGS) $(OPTFLAGS)
else
ALLFLAGS := $(CFLAGS) $(DEBFLAGS)
endif

INCLUDES := -I./incs -I./vendor/libft/include -I./vendor/libft -I./vendor/libft/include/internals -I./incs/public

# Directories

OBJ_DIR := build/obj
BIN_DIR := build/bin
LIBFT_DIR := vendor/libft
SRC_DIR := src
TEST_DIR		:= tests
BIN_TEST		:= tester

LIBFT_A := $(LIBFT_DIR)/libft.a
LIBFTPRINTF_A = $(LIBFT_DIR)/libftprintf.a
# Source and object files
SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# Output
NAME := minishell

# Colors for prettier output
RESET  := \033[0m
BOLD   := \033[1m
GREEN  := \033[32m
YELLOW := \033[33m
RED    := \033[31m

MAKEFLAGS  := --no-print-directory

all: $(BIN_DIR)/$(NAME)

# Link the final binary
$(BIN_DIR)/$(NAME): $(LIBFT_A) $(OBJS)
	@printf "$(BOLD)$(GREEN)Linking $(NAME)...$(RESET)\r" >&2
	@$(CC) $(ALLFLAGS) $(INCLUDES) $^ $(LDFLAGS) -L$(LIBFT_DIR) -lft -lreadline -o $@
	@printf "$(BOLD)$(GREEN)Linked $(NAME) successfully.\n$(RESET)" >&2

# Compile .c -> .o and generate side-by-side .d dependency file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(dir $@)
	@printf "$(YELLOW)Compiling: $< -> $@ ...$(RESET)\r" >&2
	@$(CC) $(ALLFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@
	@printf "$(GREEN)Compiled:  $< -> $@\n$(RESET)" >&2

# Include dependency files if present
-include $(DEPS)

# Build libft (in its directory)
$(LIBFT_A):
	@printf "$(BOLD)$(GREEN)Building libft...$(RESET)\r" >&2
	@$(MAKE) -C $(LIBFT_DIR) -j 8
	@printf "$(BOLD)$(GREEN)libft built.\n$(RESET)" >&2

clean:
	@printf "$(RED)Cleaning build artifacts...$(RESET)\r" >&2
	@rm -rf $(OBJ_DIR)
	@printf "$(RED)Cleaned build artifacts.\n$(RESET)" >&2

fclean: clean
	@printf "$(RED)Removing binary $(NAME)...$(RESET)\r" >&2
	@rm -f $(BIN_DIR)/$(NAME)
	@printf "$(RED)Removed binary $(NAME).\n$(RESET)" >&2
	@printf "$(BOLD)Cleaning libft...$(RESET)\r" >&2
	@$(MAKE) -C $(LIBFT_DIR) fclean
	@printf "$(BOLD)libft cleaned.\n$(RESET)" >&2

re: fclean all
	@printf "$(BOLD)$(GREEN)Rebuilt $(NAME).$(RESET)\n" >&2

test: all
	(cd $(TEST_DIR); /bin/bash $(BIN_TEST))

.PHONY: test re all clean fclean all