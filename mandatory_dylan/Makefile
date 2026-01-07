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

SRCS :=		$(shell find . -name '*.c' -type f)
INCS =		-I. -I./ft_printf/


# ...existing code...
# Build objects from sources
OBJS := $(SRCS:.c=.o)

# Default target
all: $(NAME)

# Link
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(INCS) $(OBJS) -o $(NAME)

# Compile rule
%.o: %.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

# Clean rules
clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re