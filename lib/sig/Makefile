# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/11/08 06:45:29 by dlesieur          #+#    #+#              #
#    Updated: 2025/11/08 09:27:21 by dlesieur         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# If you change SRCS, always run 'make fclean' before 'make' or 'make test'

CC			= 		cc
CFLAGS		= 		-Wall -Wextra -Werror -Wshadow -I.
DEPFLAGS	= 		-MMD -MP
NAME_LIB	=		sig
LIB_FILE = $(NAME_LIB).a
SRCS_LIB ?= $(filter-out demo_main.c,$(wildcard *.c))
OBJ_LIB = $(patsubst %.c,obj/%.o,$(SRCS_LIB))
NAME		= 		$(NAME_LIB).a
SRCS		=		$(wildcard *.c)


OBJ_DIR		=		obj
TEST_DIR	=		unit_test

TEST_SRCS	=		$(TEST_DIR)/test.c
TEST_OBJS	=		$(addprefix $(OBJ_DIR)/, $(TEST_SRCS:%.c=%.o))
TEST_BIN	=		test_$(NAME_LIB)

OBJS		=		$(addprefix $(OBJ_DIR)/, $(SRCS:%.c=%.o))
DPDC		=		$(addprefix $(OBJ_DIR)/, $(SRCS:%.c=%.d))
RM			:=		rm -rf
AR			:=		ar rcs

DEMO_SRC = demo_main.c
DEMO_OBJ = obj/demo_main.o
BIN_DIR = bin
DEMO_BIN = $(BIN_DIR)/sig_demo

# Normalize 'all' to depend on built archive
all: $(LIB_FILE) $(DEMO_BIN)

$(LIB_FILE): $(OBJ_LIB)
	@echo "[AR] $@"
	@ar rcs $@ $(OBJ_LIB)

$(OBJ_DIR)/%.o : %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/$(TEST_DIR):
	mkdir -p $(OBJ_DIR)/$(TEST_DIR)

# Build test object
$(OBJ_DIR)/$(TEST_DIR)/%.o : $(TEST_DIR)/%.c | $(OBJ_DIR)/$(TEST_DIR)
	$(CC) $(CFLAGS) -I. -c $< -o $@

# Build demo linking directly against the archive file
$(DEMO_BIN): $(LIB_FILE) $(DEMO_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -I. $(DEMO_OBJ) $(LIB_FILE) -o $(DEMO_BIN)

$(DEMO_OBJ): demo_main.c
	$(CC) $(CFLAGS) -I. -c demo_main.c -o $(DEMO_OBJ)

# Test target
test: $(NAME) $(TEST_OBJS)
	$(CC) $(CFLAGS) $(TEST_OBJS) $(OBJS) -o $(TEST_BIN)
	@echo "Run ./$(TEST_BIN) to execute tests."

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME) $(TEST_BIN)

re: fclean all

.PHONY: all clean fclean re test

-include $(DPDC)