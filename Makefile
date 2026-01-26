# Compiler and flags
CC          := cc
CFLAGS      := -Wall -Wextra -Werror -D_XOPEN_SOURCE=700 -DVERBOSE
# Ultra-fast optimization flags for OPT=1 (Clang-compatible)
OPTFLAGS    := -O3 -march=native -mtune=native -flto -ffast-math \
               -funroll-loops -finline-functions -fomit-frame-pointer \
               -fno-stack-protector -DNDEBUG -fdata-sections -ffunction-sections \
               -pipe -ftree-vectorize -fslp-vectorize -fvectorize
LDFLAGS     := -flto -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed
DEBFLAGS    := -g3 -ggdb -fsanitize=address,leak -O0
BAPTIZE_SHELL	?= hellish

# Choose flags: default = debug; pass OPT=1 when calling make to enable ultra-fast optimizations
ifdef OPT
ALLFLAGS := $(CFLAGS) $(OPTFLAGS)
LDFLAGS := $(LDFLAGS)
else
ALLFLAGS := $(CFLAGS) $(DEBFLAGS)
LDFLAGS :=
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

# Parallel compilation for faster builds
MAKEFLAGS := --no-print-directory -j$(shell nproc)

# Add this variable at the top with your other variables
COMPILED := 0

all: $(BIN_DIR)/$(NAME)

# Link the final binary
$(BIN_DIR)/$(NAME): $(LIBFT_A) $(OBJS)
	@mkdir -p $(BIN_DIR)
	@printf "\033[1;34m🔗 Linking $(NAME)...\033[0m\n"
	@$(CC) $(ALLFLAGS) $(OBJS) $(LIBFT_A) -lreadline -lm $(LDFLAGS) -o $@
	@printf "\033[1;32m✅ Built $(NAME) successfully!\033[0m\n"
ifdef OPT
	@printf "\033[1;33m⚡ Ultra-fast optimized build complete!\033[0m\n"
	@strip $@
endif

# Compile .c -> .o with inline animation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(eval COMPILED := $(shell echo $$(($(COMPILED) + 1))))
	@printf "\r\033[K\033[1;36m⚡ Compiling [$(COMPILED)/$(TOTAL)]: \033[0m$< "
ifdef OPT
	@printf "\033[1;33m(ULTRA-FAST)\033[0m"
endif
	@$(CC) $(ALLFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@
# Include dependency files if present
-include $(DEPS)

# Build libft (in its directory)
$(LIBFT_A):
	@printf "\033[1;35m📚 Building libft...\033[0m\n"
ifdef OPT
	@$(MAKE) -C vendor/libft OPT=1
else
	@$(MAKE) -C vendor/libft
endif

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

# Performance test target
perf: $(BIN_DIR)/$(NAME)
	@printf "\033[1;33m🚀 Running performance test...\033[0m\n"
	@echo "echo 'Hello World'" | time -p ./$(BIN_DIR)/$(NAME)
	@echo "ls -la | wc -l" | time -p ./$(BIN_DIR)/$(NAME)
	@printf "\033[1;32m✅ Performance test complete!\033[0m\n"

# Show optimization info
info:
ifdef OPT
	@printf "\033[1;33m⚡ ULTRA-FAST MODE ENABLED\033[0m\n"
	@printf "Optimization flags: $(OPTFLAGS)\n"
	@printf "Link flags: $(LDFLAGS)\n"
else
	@printf "\033[1;34m🐛 DEBUG MODE ENABLED\033[0m\n"
	@printf "Debug flags: $(DEBFLAGS)\n"
endif

# Ultra-fast benchmark target
benchmark: $(BIN_DIR)/$(NAME)
	@printf "\033[1;33m⚡ Running ultra-fast benchmark...\033[0m\n"
	@echo "for i in {1..100}; do echo 'test'; done | head -50" | time -p ./$(BIN_DIR)/$(NAME) >/dev/null
	@printf "\033[1;32m🏁 Benchmark complete!\033[0m\n"

.PHONY: test re all clean fclean norm my_shell perf info benchmark
