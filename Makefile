# Compiler and flags
CC          := cc
CFLAGS      := -Wall -Wextra -Werror -std=c99 -D_XOPEN_SOURCE=700 -DVERBOSE
OPTFLAGS    := -O2 -march=native
LDFLAGS     := -flto
DEBFLAGS    := -g3 -ggdb -fsanitize=address,leak
BAPTIZE_SHELL	?= hellish

# Choose flags: default = debug; pass OPT when calling make to enable optimizations
ifdef OPT
ALLFLAGS := $(CFLAGS) $(OPTFLAGS) $(LDFLAGS)
else
ALLFLAGS := $(CFLAGS) $(DEBFLAGS)
endif

INCLUDES := -I./incs -I./vendor/libft/include -I./vendor/libft -I./vendor/libft/include/internals -I./incs/public

# Directories
OBJ_DIR := build/obj
BIN_DIR := build/bin
LIBFT_DIR := vendor/libft/build/lib
SRC_DIR := src
TEST_DIR := tests
BIN_TEST := tester

LIBFT_A := $(LIBFT_DIR)/libft.a
LIBFTPRINTF_A = $(LIBFT_DIR)/libftprintf.a

# Source and object files
SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
TOTAL := $(words $(SRCS))

# Output
NAME := minishell

MAKEFLAGS := --no-print-directory

# Add this variable at the top with your other variables
COMPILED := 0

all: $(BIN_DIR)/$(NAME)

# Link the final binary
$(BIN_DIR)/$(NAME): $(LIBFT_A) $(OBJS)
	@printf "\n  \033[1;35m●\033[0m \033[1;37mLinking $(NAME)\033[0m" >&2
	@for dots in '.' '..' '...' '....' '.....'; do \
	    printf "\r  \033[1;35m●\033[0m \033[1;37mLinking $(NAME)\033[1;36m%-5s\033[0m" "$$dots" >&2; \
	    sleep 0.1; \
	done
	@$(CC) $(ALLFLAGS) $(INCLUDES) $^ $(LDFLAGS) $(LIBFT_A) -lreadline -o $@
	@printf "\r\033[K  \033[1;32m✓\033[0m \033[1;37m$(NAME) ready\033[0m\n\n" >&2

# Compile .c -> .o with inline animation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(dir $@)
	@printf "\033c\n" >&2
	@filename=$$(basename $<); \
	{ \
	    for spin in '⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧' '⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧' '⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧'; do \
	        printf "\r  \033[1;35m$$spin\033[0m \033[37mCompiling %-40s\033[0m" "$$filename" >&2; \
	        sleep 0.02; \
	    done & \
	    pid=$$!; \
	    $(CC) $(ALLFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@ 2>/dev/null; \
	    result=$$?; \
	    kill $$pid 2>/dev/null; \
	    wait $$pid 2>/dev/null; \
	    if [ $$result -eq 0 ]; then \
	        count=$$(find $(OBJ_DIR) -name "*.o" 2>/dev/null | wc -l); \
	        printf "\r  \033[1;32m✓\033[0m \033[37m%-40s\033[0m \033[1;36m%d\033[90m/\033[37m%d\033[0m" "$$filename" $$count $(TOTAL) >&2; \
	    else \
	        printf "\r  \033[1;31m✗\033[0m \033[37m%-40s\033[0m \033[1;31mFAILED\033[0m\n\n" "$$filename" >&2; \
	        $(CC) $(ALLFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@; \
	        exit 1; \
	    fi; \
	}
# Include dependency files if present
-include $(DEPS)

# Build libft (in its directory)
$(LIBFT_A):
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mBuilding libft\033[0m\n\n" >&2
	@$(MAKE) -C vendor/libft -j 8
	@printf "\n" >&2

clean:
	@printf "\n  \033[1;33m⚠\033[0m \033[1;37mCleaning build artifacts\033[0m" >&2
	@rm -rf $(OBJ_DIR)
	@printf "\r\033[K  \033[1;32m✓\033[0m \033[37mBuild artifacts cleaned\033[0m\n\n" >&2

fclean: clean
	@printf "  \033[1;33m⚠\033[0m \033[1;37mRemoving binary\033[0m" >&2
	@rm -f $(BIN_DIR)/$(NAME)
	@printf "\r\033[K  \033[1;32m✓\033[0m \033[37mBinary removed\033[0m\n\n" >&2
	@printf "  \033[1;35m●\033[0m \033[1;37mCleaning libft\033[0m" >&2
	@$(MAKE) -C vendor/libft fclean
	@printf "\r\033[K  \033[1;32m✓\033[0m \033[37mlibft cleaned\033[0m\n" >&2
	@rm -rf build
	@printf "\n" >&2

re: fclean all
	@printf "  \033[1;32m✓\033[0m \033[1;37mRebuilt $(NAME)\033[0m\n\n" >&2

test: all
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mRunning tests\033[0m\n\n" >&2
	@(cd $(TEST_DIR); /bin/bash $(BIN_TEST))

norm:
	@printf "\n  \033[1;36m▸\033[0m Running norminette" >&2; \
	output="$$( \
	    norminette 2>&1 | grep -v 'OK!' | grep -v 'US' & \
	    pid=$$!; \
	    while kill -0 $$pid 2>/dev/null; do \
	        for dots in '.' '..' '...' '....' '.....' '......'; do \
	            printf "\r  \033[1;36m▸\033[0m Running norminette\033[1;35m%-6s\033[0m" "$$dots" >&2; \
	            sleep 0.1; \
	            kill -0 $$pid 2>/dev/null || break; \
	        done; \
	    done; \
	    wait $$pid)"; \
	if [ -z "$$output" ]; then \
	    printf "\r\033[K  \033[1;32m✓\033[0m \033[1;37mNORM CHECK PASSED\033[0m\n\n"; \
	else \
	    printf "\r\033[K  \033[1;31m✗\033[0m \033[1;37mNORM VIOLATIONS:\033[0m\n\n\033[37m%s\033[0m\n\n" "$$output"; \
	fi


my_shell:
	@echo "Installing hellish shell..."
	sudo install -m 755 build/bin/minishell /usr/bin/hellish
	@echo "Registering shell..."
	./vendor/scripts/register_shell.sh
	@echo "Done. Log out and log back in to use hellish as your default shell."
	@echo "if impatient, you can use `exec /usr/bin/hellish -l`"


.PHONY: test re all clean fclean norm my_shell help
