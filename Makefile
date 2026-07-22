# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/06/06 03:00:29 by dlesieur          #+#    #+#              #
#    Updated: 2026/06/06 03:00:30 by dlesieur         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Compiler and flags
CC          := cc

# Detect compiler (best-effort) and OS
UNAME_S := $(shell uname -s 2>/dev/null)
CC_IS_CLANG := $(shell $(CC) --version 2>/dev/null | grep -qi clang && echo 1)
CC_IS_GCC   := $(shell $(CC) --version 2>/dev/null | grep -qi gcc   && echo 1)

# Includes (must be defined before CPPFLAGS assignment)
INCLUDES := -I./incs -I./vendor/libft/include -I./vendor/libft -I./vendor/libft/include/internals -I./incs/public -I./vendor/libft/srcs/memory/memalloc/slab

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
BAPTIZE_SHELL ?= hellish


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

# Allocator backend selector. SAFE=1 links against libc malloc/free (keeps
# AddressSanitizer meaningful); SAFE=0 links against the custom ft_malloc heap
# inside libft (faster, less battle-tested). The default tracks the build mode:
# the debug/ASan build is SAFE, the optimized build exercises ft_malloc. An
# explicit `SAFE=...` on the command line always wins; `make my_shell` forces 1.
# libft is built into a per-SAFE tree so the two backends never share objects.
ifdef OPT
SAFE ?= 0
else
SAFE ?= 1
endif
ifeq ($(SAFE),0)
SAFE_TAG := ft
# Force-pull ft_malloc's leak oracle from libft.a so the weakly-referenced
# malloc_live_bytes in alloc_stats.c binds (a weak ref alone won't pull an
# archive member). At SAFE=1 there is no -u, so the weak ref resolves to NULL.
LDFLAGS += -Wl,-u,malloc_live_bytes
else
SAFE_TAG := libc
endif

# Directories. Object trees are per build mode so the OPT benchmark build
# never silently reuses stale debug/ASan objects (make won't rebuild on a
# flag change alone). The binary path is shared and relinked for each mode,
# so `make bench` always times a true OPT build.
ifdef OPT
OBJ_DIR := build/obj-opt
else
OBJ_DIR := build/obj
endif
BIN_DIR := build/bin
LIBFT_DIR := vendor/libft/build-$(SAFE_TAG)/lib
SRC_DIR := src
TEST_DIR := tests
BIN_TEST := tester

LIBFT_A := $(LIBFT_DIR)/libft.a
LIBFTPRINTF_A = $(LIBFT_DIR)/libftprintf.a

# Source and object files
SRCS := $(shell find $(SRC_DIR) -name '*.c' | sort)

OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
TOTAL := $(words $(SRCS))

ifeq ($(OPT),1)
	MAKEFLAGS := --no-print-directory -j$(shell nproc)
else
	MAKEFLAGS := --no-print-directory -j1
endif

# Add this variable at the top with your other variables
COMPILED := 0

all: safe_banner $(BIN_DIR)/$(BAPTIZE_SHELL)

# Announce the active allocator before building so it is never a surprise.
safe_banner:
	@if [ "$(SAFE)" = "0" ]; then \
		printf "\n  \033[1;31m⚠  SAFE=0\033[0m \033[1;37m— custom ft_malloc heap (faster, UNSAFE).\033[0m\n" >&2; \
		printf "  \033[90mPass SAFE=1 for the libc allocator. Stability is on you.\033[0m\n\n" >&2; \
	else \
		printf "\n  \033[1;32m✓  SAFE=1\033[0m \033[1;37m— libc malloc/free.\033[0m \033[90m(OPT build defaults to SAFE=0 ft_malloc)\033[0m\n\n" >&2; \
	fi

# Link the final binary
$(BIN_DIR)/$(BAPTIZE_SHELL): $(LIBFT_A) $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) $(LIBFT_A) $(LDFLAGS) $(LDLIBS) -o $@

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

# Build libft (in its directory) into a per-SAFE tree so the libc and ft_malloc
# archives coexist and never reuse each other's objects.
$(LIBFT_A):
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mBuilding libft (-O3, %s)\033[0m\n\n" \
		"$(if $(filter ft,$(SAFE_TAG)),ft_malloc,libc)" >&2
	@$(MAKE) -C vendor/libft -j1 SAFE=$(SAFE) BUILD_DIR=build-$(SAFE_TAG)
	@printf "\n" >&2

clean:
	@printf "\n  \033[1;33m⚠\033[0m \033[1;37mCleaning build artifacts\033[0m" >&2
	@rm -rf $(OBJ_DIR)
	@printf "\r\033[K  \033[1;32m✓\033[0m \033[37mBuild artifacts cleaned\033[0m\n\n" >&2

fclean: clean
	@printf "  \033[1;33m⚠\033[0m \033[1;37mRemoving binary\033[0m" >&2
	@rm -f $(BIN_DIR)/$(BAPTIZE_SHELL)
	@printf "\r\033[K  \033[1;32m✓\033[0m \033[37mBinary removed\033[0m\n\n" >&2
	@printf "  \033[1;35m●\033[0m \033[1;37mCleaning libft\033[0m" >&2
	@$(MAKE) -C vendor/libft fclean BUILD_DIR=build-$(SAFE_TAG)
	@rm -rf vendor/libft/build-ft vendor/libft/build-libc vendor/libft/build
	@printf "\r\033[K  \033[1;32m✓\033[0m \033[37mlibft cleaned\033[0m\n" >&2
	@rm -rf build
	@printf "\n" >&2

# Run fclean THEN all, strictly in order. As plain prerequisites
# (`re: fclean all`) a parallel build (-j, which OPT/my_shell turns on) races:
# `all` starts compiling while `fclean` is still `rm -rf`-ing the build tree, so
# objects/.d files land in a directory that then vanishes ("can't create .o: No
# such file"; libft's "opening dependency file build-libc/...: No such file").
# Two separate sub-makes guarantee the ordering. OPT/SAFE are command-line
# overrides, so they propagate to the sub-makes automatically.
re:
	@$(MAKE) --no-print-directory fclean
	@$(MAKE) --no-print-directory all
	@printf "  \033[1;32m✓\033[0m \033[1;37mRebuilt $(BAPTIZE_SHELL)\033[0m\n\n" >&2

# Force a relink so the binary always matches the requested mode (debug here):
# the OPT/debug object trees are separate but the binary path is shared, and
# make won't relink on a mode switch alone.
test:
	@rm -f $(BIN_DIR)/$(BAPTIZE_SHELL)
	@$(MAKE) --no-print-directory all
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mRunning tests\033[0m\n\n" >&2
	@(cd $(TEST_DIR); /bin/bash $(BIN_TEST))

# Official speed verdict vs `bash --posix`. Always benchmarks the OPT build
# (timing the default ASan/debug build would be meaningless). Override rounds /
# scope:  make bench ROUNDS=7        make bench BENCH=micro
bench:
	@rm -f $(BIN_DIR)/$(BAPTIZE_SHELL)
	@$(MAKE) --no-print-directory OPT=1 all
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mBenchmarking hellish vs bash --posix\033[0m\n\n" >&2
	@(cd $(TEST_DIR); ROUNDS=$(ROUNDS) TIMEOUT_S=$(TIMEOUT_S) /bin/bash benchmark $(BENCH))

norm:
	@printf "\n  \033[1;36m▸\033[0m Running norminette" >&2; \
	output="$$( \
	    norminette src incs tests 2>&1 | grep -v 'OK!' | grep -v 'US' \
	        | grep -v 'Notice:' & \
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


# Install as the login shell. This is the binary you live in, so it is rebuilt
# optimized AND safe (OPT=1 SAFE=1 -> libc allocator) by default. You may force
# the custom heap with `make my_shell SAFE=0`, but then stability is on you.
my_shell:
	@$(MAKE) --no-print-directory re OPT=1 \
		SAFE=$(if $(filter command line,$(origin SAFE)),$(SAFE),1)
	@echo "Installing hellish shell (OPT=1 SAFE=$(if $(filter command line,$(origin SAFE)),$(SAFE),1))..."
	sudo install -m 755 build/bin/hellish /usr/bin/hellish
	@echo "Registering shell..."
	./vendor/scripts/register_shell.sh
	@echo "Done. Log out and log back in to use hellish as your default shell."
	@echo 'if impatient, replace the shell in THIS terminal, no relog needed:'
	@echo '    exec /usr/bin/hellish --login'

# Docker: build + run hellish FROM SOURCE in clean per-distro containers, so
# anyone can try it without chasing readline/toolchain deps on their own host.
# `docker-test` builds + smoke-tests all four distros; `docker-<distro>` drops
# you into an interactive hellish there. See docker/ and docker-compose.yml.
docker-build:
	docker compose build
docker-test:
	@chmod +x docker/test.sh && docker/test.sh
docker-alpine:
	docker compose run --rm alpine
docker-debian:
	docker compose run --rm debian
docker-ubuntu:
	docker compose run --rm ubuntu
docker-arch:
	docker compose run --rm arch
docker-clean:
	docker compose down --rmi local 2>/dev/null || true

# Cross-shell speed matrix. Build hellish + a zoo of other shells (bash, dash,
# zsh, mksh, ksh, yash, busybox ash, fish) in ONE self-contained image, then
# race them all on a portable POSIX workload set and print who is fastest and
# where hellish lands. The host needs none of those shells installed -- that is
# the whole point of doing it in docker. See tests/agnostic_bench.sh.
# Override rounds/timeout:  make agnostic-bench ROUNDS=7 TIMEOUT_S=60
agnostic-bench:
	docker build -f docker/Dockerfile.agnostic -t hellish:agnostic .
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mRacing hellish against every shell we could install\033[0m\n\n" >&2
	docker run --rm -e ROUNDS=$(ROUNDS) -e TIMEOUT_S=$(TIMEOUT_S) hellish:agnostic

# Build hellish + zsh in one image and diff the zsh-style two-argument
# `cd old new` extension against real zsh (the bash suite can't cover it).
cd-zsh-test:
	docker build -f docker/Dockerfile.zsh -t hellish:zsh .
	docker run --rm hellish:zsh

# Host-side check (no docker): `hellish --posix` must match `bash --posix` on
# the cd cases the zsh extension would otherwise change, while normal mode keeps
# the extension. Builds first so the binary is current.
cd-posix-test: all
	@chmod +x $(TEST_DIR)/cd_posix_compare.sh
	@HELLISH=$(BIN_DIR)/$(BAPTIZE_SHELL) bash $(TEST_DIR)/cd_posix_compare.sh

# Interactive multi-line history regression test (real pty): entries keep
# their multi-line text in `history` and the file, and up-arrow recall of
# loops/here-docs re-executes with bash-cmdhist semantics instead of the
# broken space-joined flattening. See tests/hist_multiline_test.py.
hist-test: all
	@python3 $(TEST_DIR)/hist_multiline_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Every libreadline entry point hellish uses, driven through a real pty. The
# golden suite cannot reach ANY of it -- every category runs `hellish -c`,
# which never enters the readline path -- so this is the only gate protecting
# completion, history recall and vi/emacs switching. Required before touching
# the readline linkage (see backlog: dlopen readline). tests/readline_paths_test.py
readline-test: all
	@python3 $(TEST_DIR)/readline_paths_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Prompt-animation vs paste regression (real pty): the idle repaint must
# freeze whenever the cursor may have left the input's first screen row
# (pasted newline, wrapped or multibyte input) instead of climbing a
# mis-computed row count and erasing the paste or the scrollback. Also
# proves the animation still runs on plain input. See tests/anim_paste_test.py.
anim-test: all
	@python3 $(TEST_DIR)/anim_paste_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Command-line option parsing (-e, -o name, +c, flags after -c, --/-,
# invalid-option status, $-, mode-dependent nounset) vs bash --posix. These
# exercise how the shell parses its own argv, which the golden -c harness
# cannot reach. Host-side, no docker. See tests/cli_opts_compare.sh.
cli-opts-test: all
	@chmod +x $(TEST_DIR)/cli_opts_compare.sh
	@HELLISH=$(BIN_DIR)/$(BAPTIZE_SHELL) bash $(TEST_DIR)/cli_opts_compare.sh

# Login-shell startup files: a login hellish must source /etc/profile (which
# runs the /etc/profile.d snippets) and then ~/.profile, exactly as bash does,
# and a non-login one must source neither. The golden -c harness only ever
# spawns non-login shells, so it cannot see this. See tests/login_profile_compare.sh.
login-test: all
	@chmod +x $(TEST_DIR)/login_profile_compare.sh
	@HELLISH=$(BIN_DIR)/$(BAPTIZE_SHELL) bash $(TEST_DIR)/login_profile_compare.sh

# Third-party conformance sweep: Oils spec tests + mksh check.t, run against
# hellish, bash --posix and dash; report in bench/conformance.md; the gate
# fails if hellish's pass count drops vs bench/baseline/. Suites are fetched
# once by bench/conformance.sh's helpers (see bench/README.md).
conformance:
	@/bin/bash bench/conformance.sh

# Dimension-split speed benchmark (startup / parse / loops / forks /
# configure) vs bash --posix and dash, via pinned hyperfine runs, followed by
# the peak-RSS dimension over the same workloads.
# Reports land in bench/results.md; methodology in bench/METHODOLOGY.md.
perf:
	@/bin/bash bench/run.sh
	@/bin/bash bench/lib/run_rss.sh

# Peak-RSS dimension on its own (run.sh must have built bench/.bin/hellish).
rss:
	@/bin/bash bench/lib/run_rss.sh

# Turn whatever harness output is on disk into bench/charts/*.svg (the images
# the README embeds). Reads every artifact it can find and skips the rest, so
# it is safe to run after a single harness; run `make perf conformance bench`
# first for a full set. Never re-runs a benchmark itself -- charting and
# measuring stay separate so a chart can always be regenerated for free.
charts:
	@python3 bench/lib/collect_data.py
	@python3 bench/lib/gen_charts.py

# Run an external, configurable 42 "minishell tester" (geoman-style) against
# the built binary, as an independent cross-check on top of `make test` and
# `make conformance`. Override the repo with `make geoman GEOMAN_URL=...`.
geoman: all
	@/bin/bash bench/lib/run_geoman.sh

.PHONY: test bench re all clean fclean norm my_shell help safe_banner \
	docker-build docker-test docker-alpine docker-debian docker-ubuntu \
	docker-arch docker-clean cd-zsh-test cd-posix-test agnostic-bench \
	hist-test readline-test anim-test conformance perf rss charts cli-opts-test \
	login-test geoman
