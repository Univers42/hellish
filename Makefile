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
SRCS :=		src/arith/eval.c\
			src/arith/helpers1.c\
			src/arith/helpers2.c\
			src/arith/helpers3.c\
			src/arith/helpers4.c\
			src/arith/helpers5.c\
			src/arith/helpers6.c\
			src/arith/helpers7.c\
			src/arith/helpers8.c\
			src/arith/lexer.c\
			src/arith/parser.c\
			src/arith/parser2.c\
			src/arith/set.c\
			src/builtins/cd_helpers1.c\
			src/builtins/cd_helpers2.c\
			src/builtins/collect_and_print_exported.c\
			src/builtins/core_builtins.c\
			src/builtins/core_builtins2.c\
			src/builtins/echo_flags.c\
			src/builtins/echo_help.c\
			src/builtins/exit.c\
			src/builtins/exit_helpers.c\
			src/builtins/export_helpers.c\
			src/builtins/export_helpers2.c\
			src/builtins/hash_builtins_dispatch.c\
			src/builtins/try_unset.c\
			src/builtins/utils.c\
			src/builtins/utils2.c\
			src/core/helpers.c\
			src/core/init.c\
			src/core/on.c\
			src/core/opt.c\
			src/core/shell.c\
			src/environment/conv.c\
			src/environment/expand.c\
			src/environment/helpers.c\
			src/environment/init_dft_env.c\
			src/environment/utils.c\
			src/environment/utils2.c\
			src/execution/exe_error.c\
			src/execution/exe_path.c\
			src/execution/execute_command.c\
			src/execution/execute_simple_list.c\
			src/execution/execute_tree_node.c\
			src/execution/find_cmd_path.c\
			src/execution/run.c\
			src/execution/execute_top_level.c\
			src/execution/res_utils.c\
			src/execution/run_utils.c\
			src/execution/exe_bg.c\
			src/execution/execute_builtin.c\
			src/execution/execute_pipeline.c\
			src/execution/execute_range.c\
			src/execution/execute_simple_command.c\
			src/execution/execute_subshell.c\
			src/execution/utils.c\
			src/execution/utils2.c\
			src/expander/assignment_to_env.c\
			src/expander/assignment_word_to_word.c\
			src/expander/expand_cmd_substitution.c\
			src/expander/expand_env_vars.c\
			src/expander/expand_export_value.c\
			src/expander/expand_node_glob.c\
			src/expander/expand_simple_cmd_assignment.c\
			src/expander/expand_tilde_token.c\
			src/expander/expand_tilde_word.c\
			src/expander/expand_token.c\
			src/expander/expand_word_single.c\
			src/expander/expander_simple_cmd.c\
			src/expander/expander_split.c\
			src/expander/process_arith_sub.c\
			src/expander/process_arith_sub_utils.c\
			src/expander/process_arith_sub_utils2.c\
			src/expander/process_cmd_sub.c\
			src/expander/replace_trailing_equal_with_full_token.c\
			src/expander/split_envvar.c\
			src/expander/word_to_string.c\
			src/expander/word_to_utils.c\
			src/expander/expand_cmd_simple_word.c\
			src/expander/process_word_token.c\
			src/expander/create_redir.c\
			src/expander/expand_cmd_sub.c\
			src/expander/utils2.c\
			src/expander/capture_subshell_output.c\
			src/expander/create_procsub_output.c\
			src/expander/expand_word.c\
			src/expander/procsub_input.c\
			src/expander/procsub_utils.c\
			src/expander/redirect_from_ast_redir.c\
			src/expander/debug.c\
			src/expander/process_simple_child.c\
			src/expander/utils.c\
			src/expander/utils3.c\
			src/glob/glob_bracket.c\
			src/glob/glob_dir.c\
			src/glob/glob_expand_utils.c\
			src/glob/glob_expand_utils2.c\
			src/glob/glob_match_at.c\
			src/glob/glob_tokenizer_utils.c\
			src/glob/glob_tokenizer.c\
			src/glob/glob_utils.c\
			src/glob/parse_bracket.c\
			src/glob/tokenizer.c\
			src/glob/ft_strcoll.c\
			src/glob/glob_expand.c\
			src/glob/glob_expand2.c\
			src/glob/glob_match.c\
			src/glob/glob_sort.c\
			src/glob/pattern_matcher.c\
			src/glob/pattern_matcher_utils.c\
			src/glob/utils.c\
			src/helpers/checked_atoi.c\
			src/helpers/free_utils.c\
			src/helpers/utils.c\
			src/helpers/var_name.c\
			src/helpers/verbose.c\SRCS = $(shell find ./srcs -name "*.c")
			src/heredoc/helpers.c\
			src/heredoc/reader.c\
			src/heredoc/helpers1.c\
			src/heredoc/collect.c\
			src/heredoc/helpers2.c\
			src/infrastructure/ast.c\
			src/infrastructure/ast_utils.c\
			src/infrastructure/ast_utils2.c\
			src/infrastructure/ast_utils3.c\
			src/infrastructure/ast_utils4.c\
			src/infrastructure/error.c\
			src/infrastructure/history.c\
			src/infrastructure/history_utils.c\
			src/infrastructure/input.c\
			src/infrastructure/input_get_more.c\
			src/infrastructure/input_get_more_utils.c\
			src/infrastructure/input_get_more_utils2.c\
			src/infrastructure/input_get_more_utils3.c\
			src/infrastructure/input_utils2.c\
			src/infrastructure/input_utils3.c\
			src/infrastructure/input_utils4.c\
			src/infrastructure/prompt_metadata.c\
			src/infrastructure/prompt_utils.c\
			src/infrastructure/prompt_utils2.c\
			src/infrastructure/rl.c\
			src/infrastructure/rl_helpers.c\
			src/infrastructure/rl_helpers2.c\
			src/infrastructure/rl_multi_line.c\
			src/infrastructure/visible_with_cstr.c\
			src/infrastructure/prompt.c\
			src/lexer/debug.c\
			src/lexer/helper2.c\
			src/lexer/helper4.c\
			src/lexer/lexer_advance.c\
			src/lexer/parse_subshell.c\
			src/lexer/print_tokens.c\
			src/lexer/print_tokens_utils.c\
			src/lexer/tables.c\
			src/lexer/tables_utils.c\
			src/lexer/tokenizer.c\
			src/lexer/parse_lexeme.c\
			src/lexer/singletons.c\
			src/lexer/helper3.c\
			src/parsing/compound_list.c\
			src/parsing/parse_arith.c\
			src/parsing/parse_cmd.c\
			src/parsing/parse_pipeline.c\
			src/parsing/parse_simple_cmd.c\
			src/parsing/parse_subshell.c\
			src/parsing/parse_tokens.c\
			src/parsing/simple_list.c\
			src/parsing/utils2.c\
			src/parsing/utils4.c\
			src/parsing/utils3.c\
			src/parsing/utils5.c\
			src/parsing/utils.c\
			src/parsing/parse_redirect.c\
			src/parsing/parser_proc_sub.c\
			src/word_splitting/reparse_assign_words.c\
			src/word_splitting/reparse_dquote.c\
			src/word_splitting/reparse_dquote_utils.c\
			src/word_splitting/reparse_envar_plain.c\
			src/word_splitting/reparse_envvar.c\
			src/word_splitting/reparse_envvar_paren.c\
			src/word_splitting/reparse_envvar_utils.c\
			src/word_splitting/reparse_escape.c\
			src/word_splitting/reparse_word.c\
			src/word_splitting/utils.c\
			src/word_splitting/utils2.c\
			src/word_splitting/verif.c

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