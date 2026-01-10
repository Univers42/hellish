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

NAME		:= shell
CC 			:= CC
RM			:= rm -rf 
WFLAGS		:= -Wall -Wextra -Werror
LDFLAGS		:= -L ./vendor/libft -lft -lreadline -lpthread
OPTFLAGS	:= -fPIE -O3 -flto
DEBUGFLAG	:= -g3 -ggdb -fsanitize=address,leak
LIBFT_PATH	:= vendor/libft
LIBFT_A		:= $(LIBFT_PATH)/libft.a

ifdef OPT
	CFLAGS := ${WFLAGS} ${OPTFLAGS}
ifdef SCHOOL
	CFLAGS := ${WFLAGS}
else
	CFLAGS := ${WFLAGS} ${DEBUGFLAG}
endif

HEADERS := -I./incs -I./$(LIBFT_PATH)/include

SRCS_DIR := .
OUT_DIR := build
OBJ_DIR	:= build/obj
BIN_DIR	:= build/bin

SOURCES := $(shell find src -type f -name "*.c" ! -path "src/libft/*")
OBJS := ${SOURCES:%.c=${OUT_DIR}/%.o}

all: ${NAME}

${NAME}: ${LIBFT_A} $(OBJS)
	${CC} ${CFLAGS} -o $^ ${OBJS} ${LIBS}

${OUT_DIR}/%.o: ${SRCS_DIR}/%.c ${HEADERS}
	mkdir -p $$(dirname $@)
	${CC} ${CFLAGS} -c $< -o $@

${OUT_DIR}:
	mkdir -p $<
	mkdir -p ${OBJ_DIR}
	mkdir -p ${BIN_DIR}

clean:
	${RM} ${OUT_DIR}

fclean: clean
	${RM} ${NAME}

test:
# for now nothing to be done

re: fclean all

.PHONY: re fclean clean all test
