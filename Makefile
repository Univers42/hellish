# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/06/06 03:00:29 by dlesieur          #+#    #+#              #
#    Updated: 2026/08/05 17:19:23 by dlesieur         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Compiler and flags
CC          := cc

# Detect compiler (best-effort) and OS
UNAME_S := $(shell uname -s 2>/dev/null)
CC_IS_CLANG := $(shell $(CC) --version 2>/dev/null | grep -qi clang && echo 1)
CC_IS_GCC   := $(shell $(CC) --version 2>/dev/null | grep -qi gcc   && echo 1)

# Platform target. Exactly one src/platform/$(TARGET)/ tree is compiled in;
# platform differences live in per-file implementations there, never in
# inline #ifdefs. Auto-detects an MSYS2/MinGW host; everything else builds
# the posix tree.
ifneq (,$(filter MINGW% MSYS%,$(UNAME_S)))
TARGET ?= win32
else
TARGET ?= posix
endif

# Includes (must be defined before CPPFLAGS assignment)
INCLUDES := -I./incs -I./incs/platform/$(TARGET) -I./vendor/libft/include -I./vendor/libft -I./vendor/libft/include/internals -I./incs/public -I./vendor/libft/srcs/memory/memalloc/slab

# macOS: two things differ and both stop the build dead at the system headers
# rather than anywhere interesting.
#
#  1. Apple ships libedit under the name libreadline. It answers -lreadline
#     and then does not have most of the GNU API this shell uses. The real
#     one comes from Homebrew and is keg-only, so its prefix has to be asked
#     for explicitly -- it is never on the default search path.
#  2. _XOPEN_SOURCE=700 pins the POSIX.1-2008 surface on glibc and musl. On
#     Apple's libc it does the opposite and HIDES the BSD extensions the
#     <sys/*.h> headers themselves depend on. _DARWIN_C_SOURCE is the
#     documented way to ask for "POSIX plus the extensions" there.
ifeq ($(UNAME_S),Darwin)
BREW_PREFIX_RL := $(shell brew --prefix readline 2>/dev/null)
ifneq ($(BREW_PREFIX_RL),)
INCLUDES += -I$(BREW_PREFIX_RL)/include
endif
CFLAGS_BASE := -Wall -Wextra -Werror -D_DARWIN_C_SOURCE -DVERBOSE
else
CFLAGS_BASE := -Wall -Wextra -Werror -D_XOPEN_SOURCE=700 -DVERBOSE
endif

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
# macOS/BSD ld does not support --gc-sections/--as-needed. What macOS does
# need is the Homebrew readline it cannot find on its own (see above).
ifeq ($(UNAME_S),Darwin)
ifneq ($(BREW_PREFIX_RL),)
LDFLAGS_BASE += -L$(BREW_PREFIX_RL)/lib
endif
endif
endif

# The shell links against libreadline for its interactive line editor. Kept in
# LDLIBS (not LDFLAGS) so it lands AFTER the objects and libft.a on the link
# line, where a static-archive-aware linker actually resolves it.
LDLIBS      := -lreadline

# Binary name. MUST stay `?=`-defined and non-empty: every rule below builds
# `$(BIN_DIR)/$(BAPTIZE_SHELL)`, and an empty value makes that the *directory*
# `build/bin/`, which already exists -- so `make` prints the banner, decides
# there is nothing to do, and exits 0 without compiling a single file.
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

# Append-only escape hatches for callers that must add flags without rewriting
# the computed sets. A command-line `CFLAGS=...` would REPLACE everything above
# (command-line variables beat makefile assignments), silently dropping -O3, the
# warning flags or the sanitizers; these add instead. `make static` uses
# EXTRA_LDFLAGS=-static, and libft already exposes the same EXTRA_CFLAGS hook.
CFLAGS  += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

# Extra -l flags, appended AFTER -lreadline. Order is load-bearing for a static
# link: readline's undefined terminal symbols are only resolved by a -lncurses
# that follows it. Putting them in EXTRA_LDFLAGS would place them before
# -lreadline (see the link rule) and leave those symbols undefined.
LDLIBS  += $(EXTRA_LDLIBS)

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

# Source and object files. src/platform/ holds one subtree per TARGET; only
# the active one is compiled (the other would multiply-define every symbol).
SRCS := $(shell find $(SRC_DIR) -path $(SRC_DIR)/platform -prune -o \
	-name '*.c' -print | sort) \
	$(shell find $(SRC_DIR)/platform/$(TARGET) -name '*.c' 2>/dev/null | sort)

# An empty SRCS is never legitimate, and the failure it produces otherwise is
# actively misleading: make happily builds an empty object list and the link
# stops at "cc: fatal error: no input files", which points nowhere near the
# cause. openSUSE Tumbleweed's base image is the real case -- it ships no
# `find` at all, so the two $(shell ...) calls above return nothing. Say so.
ifeq ($(strip $(SRCS)),)
$(error no sources found under $(SRC_DIR)/. Is `find` installed? (openSUSE and some minimal images ship without findutils.) Is the checkout complete?)
endif

OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
TOTAL := $(words $(SRCS))

# Job count. `nproc` is coreutils, so it is absent on macOS/BSD and on the
# leaner Linux images; sysctl covers macOS/BSD and getconf is POSIX.
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null \
	|| getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

# The `||` chain above only fires when a probe FAILS. A probe that succeeds
# and prints nothing -- which is what a sandboxed or cgroup-restricted
# `nproc` can do -- leaves NPROC empty, and `-j` with an empty argument
# means UNLIMITED jobs: one compiler process per source file, 487 of them,
# which is the runaway build reported in issue #43.
#
# So the emptiness is checked in pure make, with no external command. A tool
# that might be missing cannot be what guards against a missing tool -- and
# this is the same class of gap as openSUSE shipping no `find`.
#
# A non-numeric value needs no guard: make rejects `-jfoo` outright, which
# is loud and harmless. Empty is the only value that is silently dangerous.
ifeq ($(strip $(NPROC)),)
NPROC := 4
endif

# Every mode builds in parallel. The debug tree used to be pinned to -j1, which
# cost ~6x on a 6-core box for no benefit: the compile rule creates its output
# directories with `mkdir -p` (race-safe), each .o is an independent target, and
# `re` already serialises fclean before all via two sub-makes rather than by
# starving the job server. Only the inline progress animation interleaves, which
# is cosmetic. -j is exported through MAKEFLAGS, so libft inherits it too.
MAKEFLAGS := --no-print-directory -j$(NPROC)

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

# Platform files implement module seams (the fork/spawn leaves), so they —
# and only they — may see the module-private headers of the modules whose
# functions they carry.
$(OBJ_DIR)/platform/%.o: CPPFLAGS += -I./src/execution -I./src/expander \
	-I./src/builtins -I./src/job_control -I./src/infrastructure

# Compile .c -> .o with inline animation

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(dir $@)
	@printf "\033c\n" >&2
	@filename=$$(basename "$<"); \
	( \
		while :; do \
			for spin in "⠋" "⠙" "⠹" "⠸" "⠼" "⠴" "⠦" "⠧"; do \
				printf "\r  \033[1;35m%s\033[0m \033[37mCompiling %-40s\033[0m" "$$spin" "$$filename" >&2; \
				sleep 0.02; \
			done; \
		done \
	) & \
	pid=$$!; \
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c "$<" -o "$@"; \
	result=$$?; \
	kill $$pid >/dev/null 2>&1 || true; \
	wait $$pid >/dev/null 2>&1 || true; \
	if [ $$result -eq 0 ]; then \
		count=$$(find "$(OBJ_DIR)" -name "*.o" | wc -l); \
		printf "\r  \033[1;32m✓\033[0m \033[37m%-40s\033[0m \033[1;36m%d\033[90m/\033[37m%d\033[0m\n" \
			"$$filename" "$$count" "$(TOTAL)" >&2; \
	else \
		printf "\r  \033[1;31m✗\033[0m \033[37m%-40s\033[0m \033[1;31mFAILED\033[0m\n\n" \
			"$$filename" >&2; \
		exit $$result; \
	fi

# $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
# 	@mkdir -p $(BIN_DIR)
# 	@mkdir -p $(dir $@)
# 	@printf "\033c\n" >&2
# 	@filename=$$(basename $<); \
# 	{ \
# 	    for spin in '⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧' '⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧' '⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧'; do \
# 	        printf "\r  \033[1;35m$$spin\033[0m \033[37mCompiling %-40s\033[0m" "$$filename" >&2; \
# 	        sleep 0.02; \
# 	    done & \
# 	    pid=$$!; \
# 	    echo $(CC) $(CPPFLAGS) $(CFLAGS);
# 	    $(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@; \
# 	    result=$$?; \
# 	    kill $$pid 2>/dev/null; \
# 	    wait $$pid 2>/dev/null; \
# 	    if [ $$result -eq 0 ]; then \
# 	        count=$$(find $(OBJ_DIR) -name "*.o" 2>/dev/null | wc -l); \
# 	        printf "\r  \033[1;32m✓\033[0m \033[37m%-40s\033[0m \033[1;36m%d\033[90m/\033[37m%d\033[0m" "$$filename" $$count $(TOTAL) >&2; \
# 	    else \
# 	        printf "\r  \033[1;31m✗\033[0m \033[37m%-40s\033[0m \033[1;31mFAILED\033[0m\n\n" "$$filename" >&2; \
# 	        $(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@; \
# 	        exit 1; \
# 	    fi; \
# 	}

# Include dependency files if present
-include $(DEPS)

# Build libft (in its directory) into a per-SAFE tree so the libc and ft_malloc
# archives coexist and never reuse each other's objects.
# SAFE=0 needs the ft_malloc sources, but libft declares that nested
# submodule `update = none`, so `git submodule update --init --recursive`
# skips it — a fresh clone (and CI) would silently build the libc fallback
# and then die at link on the force-bound malloc_live_bytes. Materialize it
# here, with --checkout to override the none policy, before the libft build.
$(LIBFT_A):
	@if [ "$(SAFE_TAG)" = "ft" ] \
		&& [ ! -f vendor/libft/srcs/memory/ft_malloc/Makefile ]; then \
		printf "  fetching ft_malloc (declared update=none in libft)\n" >&2; \
		git -C vendor/libft submodule update --init --checkout \
			srcs/memory/ft_malloc >&2 \
		|| { printf "SAFE=0 needs ft_malloc; run: git -C vendor/libft \
submodule update --init --checkout srcs/memory/ft_malloc\n" >&2; exit 1; }; \
	fi
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mBuilding libft (-O3, %s)\033[0m\n\n" \
		"$(if $(filter ft,$(SAFE_TAG)),ft_malloc,libc)" >&2
	@$(MAKE) -C vendor/libft  SAFE=$(SAFE) BUILD_DIR=build-$(SAFE_TAG)
	@printf "\n" >&2

clean:
	@printf "\n  \033[1;33m⚠\033[0m \033[1;37mCleaning build artifacts\033[0m" >&2
	@rm -rf $(OBJ_DIR)
	@printf "\r\033[K  \033[1;32m✓\033[0m \033[37mBuild artifacts cleaned\033[0m\n\n" >&2

# SAFE is forwarded explicitly to the libft sub-make below. Only a SAFE= typed
# on the command line propagates on its own (via MAKEFLAGS); the per-mode
# default set above does not, so a plain `make fclean` used to reach libft with
# SAFE unset -- landing in its SAFE!=1 branch, running the LTO capability probe
# and printing its warning just to delete files.
fclean: clean
	@printf "  \033[1;33m⚠\033[0m \033[1;37mRemoving binary\033[0m" >&2
	@rm -f $(BIN_DIR)/$(BAPTIZE_SHELL)
	@printf "\r\033[K  \033[1;32m✓\033[0m \033[37mBinary removed\033[0m\n\n" >&2
	@printf "  \033[1;35m●\033[0m \033[1;37mCleaning libft\033[0m" >&2
	@$(MAKE) -C vendor/libft fclean SAFE=$(SAFE) BUILD_DIR=build-$(SAFE_TAG)
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

# Build the golden oracle: the exact bash the suite is defined against.
#
# `make test` diffs hellish against `bash --posix`, so the bash in PATH IS the
# specification. bash changes POSIX-visible behaviour across minor releases, and
# 5.1/5.2 vs 5.3 disagree on ~35 of these cases (printf's status on numeric
# overflow, umask symbolic validation, cd's status on too many operands, whether
# `.*` matches . and ..). Run the suite against a distro bash that is a version
# or two behind and it reports those as hellish failures. They are not. CI has
# pinned 5.3.9 since the beginning for this reason; this target gives the same
# pin locally, so `make test` here and CI agree instead of quietly disagreeing.
#
# Idempotent and cached: builds once into ~/bash-5.3.9, then exits immediately.
# tests/tester picks it up automatically (override with HELLISH_ORACLE=...).
ORACLE_PREFIX ?= $(HOME)/bash-5.3.9

oracle:
	@if [ -x "$(ORACLE_PREFIX)/bin/bash" ]; then \
		printf "\n  \033[1;32m✓\033[0m \033[1;37m%s\033[0m \033[90m(cached)\033[0m\n\n" \
			"$$($(ORACLE_PREFIX)/bin/bash --version | head -1)" >&2; \
	else \
		printf "\n  \033[1;36m▸\033[0m \033[1;37mBuilding pinned bash 5.3.9 oracle\033[0m\n\n" >&2; \
		/bin/bash tests/build_oracle.sh "$(ORACLE_PREFIX)"; \
	fi

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
# Which binary gets installed. STATIC=1 takes the container-built static one
# from dist/ instead of compiling here -- the "build it in docker, then run it
# on my machine" flow. Recursively expanded (`=`, not `:=`) because STATIC_OUT
# is defined further down the file.
MY_SHELL_BIN = $(if $(filter 1,$(STATIC)),$(STATIC_OUT),$(BIN_DIR)/$(BAPTIZE_SHELL))

my_shell:
	@if [ "$(STATIC)" = "1" ]; then \
		$(MAKE) --no-print-directory static-verify; \
	else \
		$(MAKE) --no-print-directory re OPT=1 \
			SAFE=$(if $(filter command line,$(origin SAFE)),$(SAFE),1); \
	fi
	@echo "Installing hellish shell from $(MY_SHELL_BIN)..."
	sudo install -m 755 $(MY_SHELL_BIN) /usr/bin/hellish
	@echo "Registering shell..."
	./vendor/scripts/register_shell.sh
	@echo "Done. Log out and log back in to use hellish as your default shell."
	@echo 'if impatient, replace the shell in THIS terminal, no relog needed:'
	@echo '    exec /usr/bin/hellish --login'

# The same thing on a machine where you are not root -- a lab box, a shared
# server, a 42 cluster account. `my_shell` cannot work there: it needs sudo to
# drop the binary in /usr/bin and, more fundamentally, chsh REFUSES any shell
# that is not listed in /etc/shells, and only root writes that file.
#
# So this target takes the other route every user already owns: install into
# ~/.local/bin, then append one marker-delimited block to your login shell's
# rc file that `exec`s hellish for interactive sessions. exec replaces the
# process, so this is a real shell and not an alias or a wrapper -- ps shows
# hellish, $$ is hellish, closing it closes the tab. Your passwd entry is
# never touched, which is what keeps `ssh host 'cmd'` working and leaves you a
# way back in.
#
# Idempotent: re-run it after every rebuild, it replaces the block in place.
# The binary is smoke-tested BEFORE the hook is written, so a broken build can
# never leave you with terminals that die on open.
#
#   make user-install                 OPT=1 SAFE=1 build, then install
#   make user-install STATIC=1        install the docker-built static binary
#   make user-install PREFIX=~/opt    somewhere other than ~/.local
#   make user-install RC_TARGET=~/.bashrc   hook a specific rc file
#   make user-uninstall               remove the hook and the binary
user-install:
	@if [ "$(STATIC)" = "1" ]; then \
		$(MAKE) --no-print-directory static-verify; \
	else \
		rm -f $(BIN_DIR)/$(BAPTIZE_SHELL); \
		$(MAKE) --no-print-directory all OPT=1 \
			SAFE=$(if $(filter command line,$(origin SAFE)),$(SAFE),1); \
	fi
	@PREFIX="$(if $(PREFIX),$(PREFIX),$$HOME/.local)" \
		RC_TARGET="$(RC_TARGET)" \
		./user-install.sh --bin "$(MY_SHELL_BIN)"

user-uninstall:
	@PREFIX="$(if $(PREFIX),$(PREFIX),$$HOME/.local)" \
		RC_TARGET="$(RC_TARGET)" \
		./user-install.sh --uninstall

# Build a fully static hellish in Alpine and drop it on the HOST at
# dist/hellish-linux-<arch>. This is the answer to "build it in docker, then
# run it here": an ordinary container build is NOT host-runnable (it links
# libreadline.so.8 plus the container's glibc, so it breaks on an older glibc
# and on every musl distro). Static musl has no interpreter and no .so deps, so
# the same file runs on Alpine, Debian, Ubuntu, Arch and the 42 machines alike.
#
# Needs BuildKit for --output (docker >= 18.09; standard on any current docker).
# Cross-build with ARCH=arm64 -- that path needs binfmt/qemu registered:
#   docker run --privileged --rm tonistiigi/binfmt --install all
STATIC_ARCH ?= $(shell uname -m | sed -e 's/x86_64/amd64/' -e 's/aarch64/arm64/')
STATIC_OUT  := dist/hellish-linux-$(STATIC_ARCH)

static:
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mBuilding static hellish (linux/%s)\033[0m\n\n" \
		"$(STATIC_ARCH)" >&2
	@mkdir -p dist
	DOCKER_BUILDKIT=1 docker build \
		--platform linux/$(STATIC_ARCH) \
		-f docker/Dockerfile.static \
		--target export \
		--output type=local,dest=dist/.static-$(STATIC_ARCH) \
		$(if $(BUILD_FLAGS),--build-arg BUILD_FLAGS="$(BUILD_FLAGS)",) \
		.
	@mv dist/.static-$(STATIC_ARCH)/hellish $(STATIC_OUT)
	@rm -rf dist/.static-$(STATIC_ARCH)
	@chmod 755 $(STATIC_OUT)
	@printf "\n  \033[1;32m✓\033[0m \033[1;37m%s\033[0m \033[90m(%s)\033[0m\n" \
		"$(STATIC_OUT)" "$$(du -h $(STATIC_OUT) | cut -f1)" >&2
	@printf "  \033[90m%s\033[0m\n\n" "$$(file -b $(STATIC_OUT))" >&2

# Prove the container-built binary really does run on THIS host: no dynamic
# deps, and a real command executes. `make static` alone only proves it ran
# inside Alpine; this is the part that answers "can I use it on my machine".
static-verify: static
	@printf "  \033[1;36m▸\033[0m \033[1;37mVerifying on the host\033[0m\n\n" >&2
	@if readelf -l $(STATIC_OUT) | grep -q 'program interpreter' \
		|| readelf -d $(STATIC_OUT) 2>/dev/null | grep -q 'NEEDED'; then \
		printf "  \033[1;31m✗\033[0m still dynamically linked\n" >&2; exit 1; \
	else \
		printf "  \033[1;32m✓\033[0m no interpreter, no shared deps\n" >&2; \
	fi
	@HELLISH_NO_BANNER=1 HELLISH_NO_ANIM=1 HELLISH_NO_UPDATE_CHECK=1 \
		$(STATIC_OUT) -c 'echo "  ✓ runs on host: $$(uname -s) $$(uname -m), 6*7=$$((6*7))"' >&2
	@printf "\n" >&2

# `make docker` -- the reproducible path, and the answer to every "it does not
# build on this machine" report: the toolchain comes from the image, so a host
# with a clang too old for -ffat-lto-objects, or without readline headers, or
# with a purged /goinfre docker root, is no longer your problem. It leaves a
# fully static, host-runnable binary in dist/ and verifies it HERE.
#
# `make` / `make all` deliberately stay a plain native build: this is a 42
# project, and an evaluator (or CI, or `make test`) runs make on the machine in
# front of them. Docker is the guaranteed path, not the only one.
docker: static-verify
	@printf "  \033[1;37mInstall it as your login shell with:\033[0m\n" >&2
	@printf "      \033[1;36mmake my_shell STATIC=1\033[0m\n\n" >&2

# Hermetic golden suite: build the shell AND the pinned bash 5.3.9 oracle in
# one image, then diff them there. This is the run that cannot be wrong because
# of the host -- both sides of the comparison come from the image. Use it when a
# local `make test` disagrees with CI, or on any machine whose bash is not 5.3.
#   make docker-suite                      # everything
#   make docker-suite SUITE_ARGS="redir pipe"
docker-suite:
	docker build -f docker/Dockerfile.suite -t hellish:suite .
	docker run --rm hellish:suite $(SUITE_ARGS)

# Docker: build + run hellish FROM SOURCE in clean per-distro containers, so
# anyone can try it without chasing readline/toolchain deps on their own host.
# `docker-test` builds every distro and runs docker/smoke.sh (the same 40-check
# portability workout the Platforms workflow runs) in each; `docker-<distro>`
# drops you into an interactive hellish there. See docker/ and
# docker-compose.yml for the full list -- glibc and musl, gcc and clang, five
# package managers, plus a musl+ft_malloc rung.
#
#   make docker-test                       # everything
#   docker/test.sh fedora alpine-clang     # just these two
docker-build:
	docker compose build
docker-test:
	@chmod +x docker/test.sh docker/smoke.sh && docker/test.sh
docker-alpine:
	docker compose run --rm alpine
docker-debian:
	docker compose run --rm debian
docker-ubuntu:
	docker compose run --rm ubuntu
docker-ubuntu2204:
	docker compose run --rm ubuntu2204
docker-arch:
	docker compose run --rm arch
docker-fedora:
	docker compose run --rm fedora
docker-rocky:
	docker compose run --rm rocky
docker-opensuse:
	docker compose run --rm opensuse
docker-void:
	docker compose run --rm void
docker-clean:
	docker compose down --rmi local 2>/dev/null || true

# The portability workout on its own, against whatever binary is built. Same
# script the distro containers and every Platforms CI rung run, so a failure
# reads the same everywhere.
smoke: all
	@chmod +x docker/smoke.sh && docker/smoke.sh $(BIN_DIR)/$(BAPTIZE_SHELL)

# Cross-shell speed matrix. Build hellish + a zoo of other shells (bash, dash,
# zsh, mksh, ksh, yash, busybox ash, fish) in ONE self-contained image, then
# race them all on a portable POSIX workload set and print who is fastest and
# where hellish lands. The host needs none of those shells installed -- that is
# the whole point of doing it in docker. See tests/agnostic_bench.sh.
# Override rounds/timeout:  make agnostic-bench ROUNDS=7 TIMEOUT_S=60
agnostic-bench:
	docker build -f docker/Dockerfile.agnostic -t hellish:agnostic .
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mRacing hellish against every shell we could install\033[0m\n\n" >&2
	@mkdir -p bench/.artifacts
	docker run --rm -e ROUNDS=$(ROUNDS) -e TIMEOUT_S=$(TIMEOUT_S) hellish:agnostic \
		| tee bench/.artifacts/agnostic-matrix.txt
	@python3 bench/lib/gen_matrix_chart.py

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

# The `history` builtin's OPTIONS, in a live session (issue #42). The golden
# category issue42_history_opts covers what each one does under `-c`; this
# covers the shape the bug actually had -- an interactive shell, a populated
# history file, and PROMPT_COMMAND='history -a' dumping the whole list before
# every prompt. Run it after touching builtin_history*.c.
history-opts-test: all
	@python3 $(TEST_DIR)/history_opts_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Every multi-line construct, in BOTH history modes, diffed against the pinned
# bash 5.3.9 driving the SAME keystrokes in the same pty (issue #32):
# backslash-continuation, if/elif/else, for, while, until, case, function
# definitions, brace groups, subshells, unterminated ' " and `, $( ), $(( )),
# here-docs, and a trailing | && ||. Checks the listing AND what the up-arrow
# puts back, because those are two different code paths in hellish.
# Needs the oracle: `make oracle`.
history-matrix-test: all
	@python3 $(TEST_DIR)/history_multiline_matrix.py \
		$(BIN_DIR)/$(BAPTIZE_SHELL)

# ── Every pty/regression test in tests/*.py, by DISCOVERY ────────────────────
# Not a list: a list drifts, and this one had. completion_posix_test.py lived
# in tests/ with no target and no CI job, so the POSIX command-search fix it
# guards ran nowhere from the day it landed. tests/pty_suite.sh globs the
# directory instead, so a new regression test is covered the moment it exists.
# This is what CI runs; the individual targets above stay for working on one.
pty-test: all
	@chmod +x $(TEST_DIR)/pty_suite.sh && $(TEST_DIR)/pty_suite.sh

# Non-ASCII in the PROMPT (the shortened cwd) and in TYPED INPUT (a wrapping
# line that starts with a two-byte, one-column character, plus an edit made
# at its start -- the shape reported in issue #2). Both render the pty output
# through a terminal model and compare it against what should be on screen.
widechar-test: all
	@python3 $(TEST_DIR)/widechar_prompt_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)
	@python3 $(TEST_DIR)/widechar_input_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

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

# Prompt git segment must never block on `git status` (real pty): cd into
# a repo where the scan takes seconds shows the next prompt immediately
# (the check finishes in the background and the star arrives a render
# late), TTL refreshes stay non-blocking, and normal-speed repos keep
# their exact synchronous star. See tests/git_prompt_stall_test.py.
git-prompt-test: all
	@python3 $(TEST_DIR)/git_prompt_stall_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Background jobs and the controlling terminal. POSIX only redirects an
# async list's stdin to /dev/null when job control is DISABLED; doing it
# unconditionally broke `top &` interactively ("top: failed tty get")
# instead of letting it stop on SIGTTOU the way bash does. Compares
# against bash directly, so it also notices if bash changes its mind.
bg-tty-test: all
	@python3 $(TEST_DIR)/bg_tty_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# A virtualenv prompt surviving its own deactivate. venv's activate saves
# ${PS1:-} and restores it only `if [ -n "$$_OLD_VIRTUAL_PS1" ]`, so a shell
# with no default PS1 saved "" and stayed "(venv) " forever. Runs the PS1
# handshake verbatim, then the real python3 -m venv round trip. (#39)
venv-prompt-test: all
	@python3 $(TEST_DIR)/venv_prompt_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Tab completion, built SAFE=0 ON PURPOSE. readline frees every match it is
# handed with libc free(), so a match allocated by ft_malloc is a cross-heap
# free -- and that is invisible on SAFE=1, where xmalloc IS libc malloc and
# there is no mismatch to find. Only a SAFE=0 binary can fail this gate, so
# it builds one (ASan there reports "bad-free ... not malloc()-ed"). It also
# covers the PATH scan that stopped at the first match per directory. (#40)
completion-test:
	@$(MAKE) --no-print-directory SAFE=0
	@python3 $(TEST_DIR)/completion_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# What TAB is ALLOWED to offer. A PATH element is searched for an EXECUTABLE
# FILE (POSIX XCU 2.9.1.1), but the scan matched on the directory entry name
# alone -- so a 0644 document, a subdirectory, and the "." and ".." every
# directory carries were all offered as commands, and anyone whose PATH held
# a directory that also held data had those documents listed on the first
# TAB. Same file covers the command POSITION (`ls | <TAB>` is a command in
# bash, and was a filename here) and the no-match fallback that let the cwd's
# files back in. Hermetic: PATH is one fixture directory, so the offered set
# is a closed set the test can compare exactly.
completion-posix-test: all
	@python3 $(TEST_DIR)/completion_posix_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# PS1 rendering, and the login chain that exposed it. Because hellish sets
# BASH_VERSION, Debian/Ubuntu's stock ~/.profile sources ~/.bashrc and hands
# us bash's default PS1 -- which the renderer then printed as visible text
# (":+()}\033[01;32m...") because \nnn was not an escape and ${v:+w} was read
# as a bare name. Renders that exact PS1 in a pty and checks the bytes.
ps1-render-test: all
	@python3 $(TEST_DIR)/ps1_render_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Canary (NOT a regression gate for a specific fix): drives the shell the way
# the field reports of intermittent prompt corruption describe, and asserts no
# escape sequence reached the screen without its ESC[ and no UTF-8 character
# was cut. It has never reproduced that corruption -- read its docstring
# before trusting a pass.
prompt-integrity-test: all
	@python3 $(TEST_DIR)/prompt_shortwrite_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# The pending-update badge. The loud notice is one-shot by design; this is
# the quiet marker that persists while an update is actually waiting, so a
# user who missed the notice still finds out. Also covers the \U escape.
update-badge-test: all
	@python3 $(TEST_DIR)/update_badge_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Release/update configuration, checked offline: the GitHub slug baked into
# version.h must match the repository we actually push to, every
# distribution channel (install.sh, npm, Dockerfile, docs) must agree with
# it, and npm's version must track HELLISH_VERSION. No runtime test can
# reach any of this -- a wrong slug only shows up as a 404 at update time.
update-config-test:
	@bash $(TEST_DIR)/update_config_check.sh

# The `help` builtin. The load-bearing check is that EVERY builtin in the
# dispatch table has a help entry -- documentation rots by omission, and
# deriving the expected set from hash_builtins_dispatch.c makes that
# impossible to do quietly.
help-test: all
	@bash $(TEST_DIR)/help_test.sh $(BIN_DIR)/$(BAPTIZE_SHELL)

# The update path end to end against a LOCAL fake release server: discovery,
# download, sha256, atomic replace, and every rejection path (bad checksum,
# truncated asset, unreachable source). Nothing is installed system-wide and
# no network is touched. update_ui_test drives a pty for the two interactive
# requirements: the header must be lazy, and a discovered update must never
# disturb a line the user is typing.
update-test: all
	@python3 $(TEST_DIR)/update_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)
	@python3 $(TEST_DIR)/update_ui_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# The prompt prefix must reach the tty in ONE write (real pty). Streaming it
# byte by byte let the line discipline echo type-ahead INTO a colour escape;
# a letter is a valid CSI final byte, so the sequence ended early and its
# tail printed as text (`38;2;112`). See tests/prompt_atomic_test.py.
prompt-atomic-test: all
	@chmod +x $(TEST_DIR)/prompt_atomic_test.py
	@python3 $(TEST_DIR)/prompt_atomic_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# The prompt against a NON-BLOCKING terminal (issue #34). O_NONBLOCK lives on
# the open file description the shell shares with every program it launches,
# so one tool that sets it and never restores it leaves the shell writing to
# a non-blocking tty. tty_write_all used to treat the resulting EAGAIN as a
# hard error and drop the WHOLE frame. The last two checks fill the output
# queue for real before the shell draws, so they fail outright if the EAGAIN
# wait is removed. See tests/nonblock_tty_test.py.
nonblock-tty-test: all
	@chmod +x $(TEST_DIR)/nonblock_tty_test.py
	@python3 $(TEST_DIR)/nonblock_tty_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# /dev/tcp and /dev/udp redirections vs bash --posix. Brings up its own TCP
# and UDP peer, so it needs python3 but no network access.
net-redir-test: all
	@chmod +x $(TEST_DIR)/net_redir_test.py
	@python3 $(TEST_DIR)/net_redir_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

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
	@python3 bench/lib/gen_matrix_chart.py

# Run an external, configurable 42 "minishell tester" (geoman-style) against
# the built binary, as an independent cross-check on top of `make test` and
# `make conformance`. Override the repo with `make geoman GEOMAN_URL=...`.
geoman: all
	@/bin/bash bench/lib/run_geoman.sh

.PHONY: test bench re all clean fclean norm my_shell help safe_banner \
	static static-verify \
	docker-build docker-test docker-alpine docker-debian docker-ubuntu \
	docker-arch docker-fedora docker-rocky docker-opensuse docker-void \
	smoke docker-clean cd-zsh-test cd-posix-test agnostic-bench \
	hist-test history-opts-test history-matrix-test pty-test \
	completion-test completion-posix-test \
	readline-test anim-test git-prompt-test \
	prompt-atomic-test \
	bg-tty-test prompt-integrity-test update-badge-test nonblock-tty-test \
	update-config-test update-test help-test \
	conformance perf rss \
	charts cli-opts-test net-redir-test login-test geoman oracle docker-suite docker widechar-test \
	user-install user-uninstall
