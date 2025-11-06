# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/10/23 19:03:21 by dlesieur          #+#    #+#              #
#    Updated: 2025/11/06 21:27:43 by dlesieur         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Generic project Makefile — discovers sources in srcs, builds objects in .objs/, and includes dependency files

# Submodule / dependency management
SUBMODULE_DIR	:= lib/libft
SUBMODULE_REPO	:= git@github.com:Univers42/libft.git
SUBMODULE_LIB	:= $(SUBMODULE_DIR)/libft.a
REMOTE_HOME		:= $(shell git remote -v | awk 'NR==3 {print $$1}')
CURRENT_BRANCH	:= $(shell git branch --show-current)
REMOTE_CAMPUS	:= $(shell git remote -v | awk 'NR==2 {print $$1}')
MSG				:=
BRANCH			?=

include	variables.mk
-include lib/libft/build/colors.mk
-include lib/libft/build/common.mk
-include lib/libft/build/debug.mk

SRCDIR := srcs
OBJDIR := .objs
DEPDIR := .deps

SRCS := $(shell find $(SRCDIR) -type f -name '*.c' -print)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(patsubst $(OBJDIR)/%.o,$(DEPDIR)/%.d,$(OBJS))

# detect libft archive at link-time (evaluated when used)
LIBFT_A = $(firstword $(wildcard $(SUBMODULE_DIR)/libft.a $(SUBMODULE_DIR)/build/libft.a $(SUBMODULE_DIR)/lib/libft.a))

MINISHELL_A := libminishell.a

# Default target: build the program (libft will be ensured first)
all: $(NAME)

# Ensure libft.a exists; if not, try to build it using the libft Makefile (simple and explicit)
LIBFT_STAMP := .libft.stamp

$(LIBFT_STAMP): $(SUBMODULE_LIB)
	@if [ ! -f "$@" ]; then \
		echo "[deps] libft.a is now available"; \
		touch $@; \
	fi

$(SUBMODULE_LIB):
	echo "[DEPS] ensuring libft.a..."
	@if [ -f "$(SUBMODULE_LIB)" ]; then																										\
		echo "[deps] libft.a found: $(SUBMODULE_LIB)";																						\
	else 																																	\
		if [ -d "$(SUBMODULE_DIR)" ]; then																									\
			if [ -f "$(SUBMODULE_DIR)/Makefile" ]; then																						\
				echo "[deps] building libft in $(SUBMODULE_DIR)";																			\
				$(MAKE) -C $(SUBMODULE_DIR) || true;																						\
			else																															\
				echo "[deps] libft directory exists but no Makefile — cannot build automatically";											\
			fi;																																\
		else 																																\
			echo "[deps] libft not present — trying to init submodule"; 																	\
			git submodule update --init --recursive $(SUBMODULE_DIR) || git submodule add $(SUBMODULE_REPO) $(SUBMODULE_DIR) || true;		\
			if [ -f "$(SUBMODULE_DIR)/Makefile" ]; then 																					\
				echo "[deps] building libft in $(SUBMODULE_DIR)";																			\
				$(MAKE) -C $(SUBMODULE_DIR) || true;																						\
			fi;																																\
		fi;																																	\
	fi;																																		\
	if [ ! -f "$(SUBMODULE_LIB)" ]; then																									\
		echo "[deps] libft.a still missing — continuing without libft (link may fail)";														\
	fi;																																		\
	if [ -z "$$MAKE_RESTARTED" ] && { [ -f "$(SUBMODULE_DIR)/build/common.mk" ] || [ -f "$(SUBMODULE_DIR)/build/colors.mk" ]; }; then		\
		echo "[deps] libft: build helpers available; re-running make to load them (once)";													\
		$(MAKE) MAKE_RESTARTED=1 all;																										\
		exit 0;																																\
	fi

# Build static library from all project objects
$(MINISHELL_A): $(OBJS)
	@echo "[AR] $@"
	@ar rcs $@ $(OBJS)

# Link the final binary from your static lib and libft
$(NAME): $(LIBFT_STAMP) $(MINISHELL_A)
	$(call print_status,$(GREEN),LINK,$@)
	@echo "[link] linking $(NAME)"
	$(CC) $(CFLAGS) -o $@ $(MINISHELL_A) $(LIBFT_A) $(LDFLAGS)

# Compile C source to object files and emit dependency files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@) $(DEPDIR)/$(dir $*)
	$(call print_status,$(CYAN),COMPILING,$<)
	@$(CC) $(CFLAGS) $(OPTFLAGS) -c $< -o $@ $(PREPROCFLAGS) $(DEPDIR)/$*.d -I$(SUBMODULE_DIR)/include -I./incs

# Include dependency files if present
-include $(DEPS)

configure:
	@chmod +x scripts/* scripts/*/* 2>/dev/null || true
	@bash -c 'if [ -x "scripts/install-hooks.sh" ]; then \
		./scripts/install-hooks.sh >/dev/null 2>&1 && printf "%s\n" "$(BRIGHT_CYAN)$(BOLD)$(LOG_PREFIX)$(RESET) [$(STATE_COLOR_OK)$(BOLD)OK$(RESET)] : Project hooks configured successfully!"; \
	else \
		printf "%s\n" "$(BRIGHT_CYAN)$(BOLD)$(LOG_PREFIX)$(RESET) [$(STATE_COLOR_WARN)$(BOLD)WARN$(RESET)] : scripts/install-hooks.sh not found or not executable."; \
	fi'

update: configure
	$(call log_info, updating submodule to latest remote version...)
	git submodule update --init --recursive --remote --merge $(SUBMODULE_DIR) || exit 1
	git pull

push_home: ffclean
	@# require MSG to be provided to avoid accidental empty commits
	@if [ -z "$(MSG)" ]; then \
		printf "ERROR: pass a commit message with MSG=\"your message\"\n"; \
		exit 1; \
	fi
	@git add .
	@git commit -m "$(MSG)"
	@git push $(REMOTE_HOME) $(CURRENT_BRANCH)

push_campus: ffclean
	@if [ -z "$(MSG)" ]; then \
		printf "ERROR: pass a commit message with MSG=\"your message\"\n"; \
		exit 1; \
	fi
	@git add .
	@git commit -m "$(MSG)"
	@git push $(REMOTE_CAMPUS) --all

clean:
	$(call print_status,$(RED),CLEAN,Removing objects and archives)
	@rm -rf $(OBJDIR) $(DEPDIR) $(MINISHELL_A)
	$(call log_info,$(NAME) cleaned up : you can use $(BOLD_CYAN)$(UNDERLINE)$(ITALIC)$(NAME)$(RESET))

fclean: clean
	$(call print_status,$(RED),FCLEAN,Removing binary)
	@rm -f $(NAME) .libft.stamp
	$(call log_warn,$(NAME) cleaned up : you cannot use anymore $(NAME), you have to)

ffclean: fclean
	$(call print_status,$(RED),FFCLEAN,Removing all gen files)
	@if [ -d "$(SUBMODULE_DIR)" ]; then					\
		$(MAKE) -C $(SUBMODULE_DIR) fclean || true;		\
	fi
	$(call log_ok, the libft.a was erased succesfully)
	$(call log_note, use $(GREEN)make$(RESET) to start the project)

re: fclean all

fre: ffclean all

# Build configurations
.PHONY: help-only debug-only all-features minimal test-lexer

# Minimal build with only help
help-only:
	$(MAKE) CFLAGS="$(CFLAGS) -DFEATURE_HELP_ONLY" all

# Debug-only build
debug-only:
	$(MAKE) CFLAGS="$(CFLAGS) -DFEATURE_DEBUG_ONLY" all

# Full-featured build (default)
all-features:
	$(MAKE) CFLAGS="$(CFLAGS) -DFEATURE_HELP -DFEATURE_DEBUG -DFEATURE_LOGIN -DFEATURE_POSIX -DFEATURE_PRETTY_PRINT" all

# Minimal core only
minimal:
	$(MAKE) CFLAGS="$(CFLAGS) -UFEATURE_HELP -UFEATURE_DEBUG -UFEATURE_LOGIN -UFEATURE_POSIX -UFEATURE_PRETTY_PRINT" all

test-lexer: $(NAME)
	@echo "[test] running lexer tests..."
	@python3 srcs/test/run_lexer_tests.py

# Verbose runner with optional filters:
# make test-lexer-verbose [ID=L123] [GREP=pattern] [LOG=path]
test-lexer-verbose: $(NAME)
	@echo "[test] running lexer tests (verbose)..."
	@python3 srcs/test/run_lexer_tests.py \
		$(if $(ID),--id $(ID),) \
		$(if $(GREP),--grep "$(GREP)",) \
		--verbose $(if $(LOG),--log-file "$(LOG)",)

# Customize this to whatever runs your tests without the tee:
# Example: a raw test target that prints to stdout (colors or not)
test-lexer-raw:
	@echo "Please set test-lexer-raw to run your lexer tests"; exit 2
	# Example:
	# python3 srcs/test/run_lexer_tests_raw.py

# Pretty console + clean log target:
test-lexer:
	@python3 srcs/test/run_lexer_tests.py

finish:
	git add .
	git commit
	git push
	//we save here the branch before checkout
	git checkout develop
	git merge $(BRANCH_CHECKED_OUT)
	git push
	MAKE -C rm_branch BRANCH=$(BRANCH_CHECKED_OUT)
	
rm_branch:
	git branch -D $(BRANCH)
	git push origin --delete $(BRANCH)

publish:
	@export GIT_PUBLISH=1; \
	BR=$$(git branch --show-current); \
	MSG_VAL='$(subst ','\'',$(MSG))'; \
	if [ -z "$$BR" ]; then echo "fatal: branch name required"; exit 128; fi; \
	if [ -z "$$MSG_VAL" ]; then echo "fatal: provide commit message via MSG=\"...\""; exit 2; fi; \
	echo "[publish] current branch: $$BR"; \
	git add .; \
	if ! git diff --cached --quiet; then \
		git commit -m "$$MSG_VAL"; \
	else \
		echo "[publish] nothing to commit"; \
	fi; \
	git push origin "$$BR"; \
	echo "[publish] switching to develop"; \
	git checkout develop; \
	git pull --ff-only; \
	echo "[publish] merging $$BR -> develop"; \
	git merge --no-ff "$$BR"; \
	git push origin develop; \
	echo "[publish] deleting branch $$BR locally and on origin"; \
	git branch -D "$$BR"; \
	git push origin --delete "$$BR" || true; \
	echo "[publish] done (on branch: develop)"

# Auto-run set-hooks on first use (after clone)
ifeq ($(wildcard .init.stamp),)
.PHONY: _auto_init
_auto_init:
	#$(MAKE) set-hooks
	@touch .init.stamp

# Prepend _auto_init to all main targets
all: _auto_init $(NAME)
clean: _auto_init
fclean: _auto_init
ffclean: _auto_init
re: _auto_init
fre: _auto_init
endif

.PHONY: all clean fclean ffclean re push_campus push_home configure publish help

help:
	@echo ""
	@echo "$(BRIGHT_CYAN)$(BOLD)=== sh42 Makefile Targets ===$(RESET)"
	@echo ""
	@echo "$(BOLD)Build Targets:$(RESET)"
	@echo "  $(GREEN)make$(RESET) or $(GREEN)make all$(RESET)        - Build the minishell binary (default target)"
	@echo "  $(GREEN)make re$(RESET)                  - Clean and rebuild from scratch"
	@echo "  $(GREEN)make fre$(RESET)                 - Full clean (includes libft) and rebuild"
	@echo ""
	@echo "$(BOLD)Clean Targets:$(RESET)"
	@echo "  $(GREEN)make clean$(RESET)              - Remove object files and archives"
	@echo "  $(GREEN)make fclean$(RESET)             - Remove binary and object files"
	@echo "  $(GREEN)make ffclean$(RESET)            - Full clean (also cleans libft submodule)"
	@echo ""
	@echo "$(BOLD)Build Configuration Targets:$(RESET)"
	@echo "  $(GREEN)make help-only$(RESET)          - Build with FEATURE_HELP_ONLY"
	@echo "  $(GREEN)make debug-only$(RESET)         - Build with FEATURE_DEBUG_ONLY"
	@echo "  $(GREEN)make all-features$(RESET)       - Build with all features enabled"
	@echo "  $(GREEN)make minimal$(RESET)            - Build minimal core (no optional features)"
	@echo ""
	@echo "$(BOLD)Testing Targets:$(RESET)"
	@echo "  $(GREEN)make test-lexer$(RESET)         - Run lexer tests"
	@echo "  $(GREEN)make test-lexer-verbose$(RESET) - Run lexer tests with verbose output"
	@echo "    Options: ID=L123 GREP=pattern LOG=path"
	@echo "  $(GREEN)make test-lexer-raw$(RESET)     - Run raw lexer tests (prints to stdout)"
	@echo ""
	@echo "$(BOLD)Git/Push Targets:$(RESET)"
	@echo "  $(GREEN)make push_home$(RESET)          - Push to home remote (requires MSG=\"...\")"
	@echo "  $(GREEN)make push_campus$(RESET)        - Push all to campus remote (requires MSG=\"...\")"
	@echo "  $(GREEN)make publish$(RESET)            - Publish feature branch: commit, merge to develop,"
	@echo "                            delete branch (requires MSG=\"...\")"
	@echo ""
	@echo "$(BOLD)Maintenance Targets:$(RESET)"
	@echo "  $(GREEN)make configure$(RESET)          - Set up git hooks and scripts"
	@echo "  $(GREEN)make update$(RESET)             - Update libft submodule to latest"
	@echo ""
	@echo "$(BRIGHT_CYAN)$(BOLD)Examples:$(RESET)"
	@echo "  $(CYAN)make$(RESET)                      # Build the project"
	@echo "  $(CYAN)make re$(RESET)                   # Clean and rebuild"
	@echo "  $(CYAN)make publish MSG=\"feat: add feature\"$(RESET)  # Publish feature branch"
	@echo "  $(CYAN)make test-lexer-verbose ID=L1$(RESET)  # Run specific lexer test"
	@echo ""
