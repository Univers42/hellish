# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: marvin <marvin@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/07 14:17:54 by marvin            #+#    #+#              #
#    Updated: 2026/01/07 14:17:54 by marvin           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CC =		cc
CFLAGS =	-Wall -Wextra -Werror
RM =		rm -rf

NAME =		minishell
OBJ_DIR =	obj

SRCS :=		$(shell find . -name '*.c' -type f)
INCS =		-I. -I./ft_printf/


# Build objects into $(OBJ_DIR), preserving subdirectory structure.
OBJS := $(patsubst ./%, $(OBJ_DIR)/%, $(SRCS:.c=.o))

# Default target
all: $(NAME)

# Link
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(INCS) $(OBJS) -o $(NAME)

# Compile rule: build obj/<path>.o from ./<path>.c, create directories as needed
$(OBJ_DIR)/%.o: ./%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

# Clean rules
clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re