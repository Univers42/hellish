# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/11/08 06:45:29 by dlesieur          #+#    #+#              #
#    Updated: 2025/11/10 19:24:17 by dlesieur         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CC              := cc
CFLAGS          := -Wall -Wextra -Werror -Wshadow -I.
DEPFLAGS        := -MMD -MP

NAME_LIB        := sig
LIB_FILE        := $(NAME_LIB).a

OBJ_DIR         := obj
BIN_DIR         := bin
PROGRAMS_DIR    := programs

# Root sources (library) = all .c in this directory (exclude programs/)
SRCS_LIB        := $(wildcard *.c)
OBJ_LIB         := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS_LIB))
DEP_LIB         := $(OBJ_LIB:.o=.d)

# Programs (each has one source for now)
PROG_SRC_demo_main      := $(PROGRAMS_DIR)/demo_main.c
PROG_SRC_demo_posix     := $(PROGRAMS_DIR)/posix_example.c
PROG_SRC_demo_sigaction := $(PROGRAMS_DIR)/demo_sigaction.c
PROG_SRC_demo_kill      := $(PROGRAMS_DIR)/demo_kill.c
PROG_SRC_demo_leak      := $(PROGRAMS_DIR)/demo_leak.c
PROG_SRC_demo_zombie    := $(PROGRAMS_DIR)/demo_zombie.c
PROG_SRC_demo_shell     := $(PROGRAMS_DIR)/demo_shell.c
PROG_SRC_demo_signals   := $(PROGRAMS_DIR)/demo_signals.c

PROG_OBJ_demo_main      := $(OBJ_DIR)/$(PROGRAMS_DIR)/demo_main.o
PROG_OBJ_demo_posix     := $(OBJ_DIR)/$(PROGRAMS_DIR)/posix_example.o
PROG_OBJ_demo_sigaction := $(OBJ_DIR)/$(PROGRAMS_DIR)/demo_sigaction.o
PROG_OBJ_demo_kill      := $(OBJ_DIR)/$(PROGRAMS_DIR)/demo_kill.o
PROG_OBJ_demo_leak      := $(OBJ_DIR)/$(PROGRAMS_DIR)/demo_leak.o
PROG_OBJ_demo_zombie    := $(OBJ_DIR)/$(PROGRAMS_DIR)/demo_zombie.o
PROG_OBJ_demo_shell     := $(OBJ_DIR)/$(PROGRAMS_DIR)/demo_shell.o
PROG_OBJ_demo_signals   := $(OBJ_DIR)/$(PROGRAMS_DIR)/demo_signals.o

PROG_BIN_demo_main      := $(BIN_DIR)/demo_main
PROG_BIN_demo_posix     := $(BIN_DIR)/demo_posix
PROG_BIN_demo_sigaction := $(BIN_DIR)/demo_sigaction
PROG_BIN_demo_kill      := $(BIN_DIR)/demo_kill
PROG_BIN_demo_leak      := $(BIN_DIR)/demo_leak
PROG_BIN_demo_zombie    := $(BIN_DIR)/demo_zombie
PROG_BIN_demo_shell     := $(BIN_DIR)/demo_shell
PROG_BIN_demo_signals   := $(BIN_DIR)/demo_signals

PROGRAM_BINS := \
	$(PROG_BIN_demo_main) \
	$(PROG_BIN_demo_posix) \
	$(PROG_BIN_demo_sigaction) \
	$(PROG_BIN_demo_kill) \
	$(PROG_BIN_demo_leak) \
	$(PROG_BIN_demo_zombie) \
	$(PROG_BIN_demo_shell) \
	$(PROG_BIN_demo_signals)

# Readline libs for shell demo
READLINE_LIBS := -lreadline -lncurses

# Default
all: $(LIB_FILE) demo

# Archive
$(LIB_FILE): $(OBJ_LIB)
	@echo "[AR] $@"
	@ar rcs $@ $(OBJ_LIB)

# Root objects
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Program objects
$(OBJ_DIR)/$(PROGRAMS_DIR)/%.o: $(PROGRAMS_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -I. -c $< -o $@

# Link programs
$(PROG_BIN_demo_main): $(LIB_FILE) $(PROG_OBJ_demo_main) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I. $(PROG_OBJ_demo_main) $(LIB_FILE) -o $@

$(PROG_BIN_demo_posix): $(LIB_FILE) $(PROG_OBJ_demo_posix) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I. $(PROG_OBJ_demo_posix) $(LIB_FILE) -o $@

$(PROG_BIN_demo_sigaction): $(LIB_FILE) $(PROG_OBJ_demo_sigaction) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I. $(PROG_OBJ_demo_sigaction) $(LIB_FILE) -o $@

$(PROG_BIN_demo_kill): $(LIB_FILE) $(PROG_OBJ_demo_kill) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I. $(PROG_OBJ_demo_kill) $(LIB_FILE) -o $@

$(PROG_BIN_demo_leak): $(LIB_FILE) $(PROG_OBJ_demo_leak) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I. $(PROG_OBJ_demo_leak) $(LIB_FILE) -o $@

$(PROG_BIN_demo_zombie): $(LIB_FILE) $(PROG_OBJ_demo_zombie) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I. $(PROG_OBJ_demo_zombie) $(LIB_FILE) -o $@

$(PROG_BIN_demo_shell): $(LIB_FILE) $(PROG_OBJ_demo_shell) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I. $(PROG_OBJ_demo_shell) $(LIB_FILE) $(READLINE_LIBS) -o $@

$(PROG_BIN_demo_signals): $(LIB_FILE) $(PROG_OBJ_demo_signals) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I. $(PROG_OBJ_demo_signals) $(LIB_FILE) -o $@

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Public demo targets
demo_main: $(PROG_BIN_demo_main)
	@echo "Built $@ -> $(PROG_BIN_demo_main)"

demo_posix: $(PROG_BIN_demo_posix)
	@echo "Built $@ -> $(PROG_BIN_demo_posix)"

demo_sigaction: $(PROG_BIN_demo_sigaction)
	@echo "Built $@ -> $(PROG_BIN_demo_sigaction)"

demo_kill: $(PROG_BIN_demo_kill)
	@echo "Built $@ -> $(PROG_BIN_demo_kill)"

demo_leak: $(PROG_BIN_demo_leak)
	@echo "Built $@ -> $(PROG_BIN_demo_leak)"

demo_zombie: $(PROG_BIN_demo_zombie)
	@echo "Built $@ -> $(PROG_BIN_demo_zombie)"

demo_shell: $(PROG_BIN_demo_shell)
	@echo "Built $@ -> $(PROG_BIN_demo_shell)"

demo_signals: $(PROG_BIN_demo_signals)
	@echo "Built $@ -> $(PROG_BIN_demo_signals)"

demo: $(PROGRAM_BINS)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(LIB_FILE) $(BIN_DIR) test_$(NAME_LIB)

re: fclean all

.PHONY: all clean fclean re demo demo_main demo_posix demo_sigaction demo_kill demo_leak demo_zombie demo_shell demo_signals

-include $(DEP_LIB)