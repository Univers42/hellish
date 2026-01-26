# Compiler and flags
CC          := cc

# Detect compiler (best-effort) and OS
UNAME_S := $(shell uname -s 2>/dev/null)
CC_IS_CLANG := $(shell $(CC) --version 2>/dev/null | grep -qi clang && echo 1)
CC_IS_GCC   := $(shell $(CC) --version 2>/dev/null | grep -qi gcc   && echo 1)

# Includes (must be defined before CPPFLAGS assignment)
INCLUDES := -I./incs -I./vendor/libft/include -I./vendor/libft -I./vendor/libft/include/internals -I./incs/public

# Base compile flags
CFLAGS_BASE := -Wall -Wextra -Werror -D_XOPEN_SOURCE=700 -DVERBOSE

# Debug / sanitize flags
DEBFLAGS    := -g3 -ggdb -O0
SANFLAGS    := -fsanitize=address,leak

# Optimization flags (portable-ish)
OPTFLAGS_COMMON := -O3 -ffast-math -funroll-loops -finline-functions -fomit-frame-pointer -DNDEBUG -pipe
# GCC-only / Clang-only extras
OPTFLAGS_GCC   := -fdata-sections -ffunction-sections
OPTFLAGS_CLANG := -fdata-sections -ffunction-sections

# LTO flags
LTO_CFLAGS  := -flto
LTO_LDFLAGS := -flto

# Linker flags (do not mix into compile flags)
LDFLAGS_BASE :=
ifeq ($(UNAME_S),Linux)
LDFLAGS_BASE += -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed
else
# macOS/BSD ld does not support --gc-sections/--as-needed
LDFLAGS_BASE +=
endif

LDLIBS      := -lreadline
BAPTIZE_SHELL\t?= hellish

# Choose flags: default = debug; pass OPT=1 when calling make to enable optimizations
ifdef OPT
CPPFLAGS := $(INCLUDES)
CFLAGS   := $(CFLAGS_BASE) $(OPTFLAGS_COMMON) \
            $(if $(CC_IS_GCC),$(OPTFLAGS_GCC),) \
            $(if $(CC_IS_CLANG),$(OPTFLAGS_CLANG),) \
            $(LTO_CFLAGS)
LDFLAGS  := $(LDFLAGS_BASE) $(LTO_LDFLAGS)
else
CPPFLAGS := $(INCLUDES)
CFLAGS   := $(CFLAGS_BASE) $(DEBFLAGS) $(SANFLAGS)
LDFLAGS  := $(LDFLAGS_BASE) $(SANFLAGS)
endif

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

ifeq ($(OPT),1)
	MAKEFLAGS := --no-print-directory -j$(shell nproc)
else
	MAKEFLAGS := --no-print-directory -j1
endif

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
	@$(CC) $(OBJS) $(LIBFT_A) $(LDFLAGS) $(LDLIBS) -o $@
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
	    $(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@ 2>/dev/null; \
	    result=$$?; \
	    kill $$pid 2>/dev/null; \
	    wait $$pid 2>/dev/null; \
	    if [ $$result -eq 0 ]; then \
	        count=$$(find $(OBJ_DIR) -name "*.o" 2>/dev/null | wc -l); \
	        printf "\r  \033[1;32m✓\033[0m \033[37m%-40s\033[0m \033[1;36m%d\033[90m/\033[37m%d\033[0m" "$$filename" $$count $(TOTAL) >&2; \
	    else \
	        printf "\r  \033[1;31m✗\033[0m \033[37m%-40s\033[0m \033[1;31mFAILED\033[0m\n\n" "$$filename" >&2; \
	        $(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@; \
	        exit 1; \
	    fi; \
	}
	
# Include dependency files if present
-include $(DEPS)

# Build libft (in its directory)
$(LIBFT_A):
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mBuilding libft\033[0m\n\n" >&2
	@$(MAKE) -C vendor/libft -j20
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