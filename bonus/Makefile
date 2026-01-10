TARGET := minishell

SRCS_DIR := .
OUT_DIR := build

SOURCES := $(shell find src -type f -name "*.c" ! -path "src/libft/*")

HEADERS := ./src/env/env.h\
./src/builtins/builtins.h\
./src/shell.h

OBJS := ${SOURCES:%.c=${OUT_DIR}/%.o}

ifdef OPT
	CFLAGS := -fPIE -Wall -Wextra -Werror -O3 -flto ${FLAGS}
else
	CFLAGS := -Wall -Wextra -Werror -g3 -ggdb -fsanitize=address,leak ${FLAGS}
endif

LIBFT_A := src/libft.a

LIBS := -L ./src -lft -lreadline

CC := cc

all: ${TARGET} ${headers}

${TARGET}: Makefile build ${OBJS} ${LIBFT_A}
	${CC} ${CFLAGS} -o ${TARGET} ${OBJS} ${LIBS}

${OUT_DIR}/%.o: ${SRCS_DIR}/%.c ${HEADERS}
	mkdir -p $$(dirname $@)
	${CC} ${CFLAGS} -c $< -o $@

${OUT_DIR}:
	mkdir -p build

clean:
	rm -rf ${OUT_DIR}

fclean: clean
	rm -rf ${TARGET}

re:: fclean
re:: all

run: all
	./${TARGET}

.PHONY: test clean fclean re
