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

# ── Shared make fragments ────────────────────────────────────────────────────
# The logging wrappers (log_ok, log_info, log_warn, log_error, log_title,
# print_status) come from libft so the two projects sound the same in a
# terminal. mk/colors.mk supplies the palette they interpolate and explains at
# length why libft's global_conf.mk is NOT included: it mixes the palette with
# CC, CFLAGS, OBJ_DIR, LDFLAGS and RM, and would quietly redefine this build.
#
# Guarded with wildcard because vendor/libft is a submodule: a fresh clone
# without `--recursive` still has to be able to run `make help` and be told
# what to do about it, rather than dying on a missing include.
include mk/colors.mk

LIBFT_MK := vendor/libft/mk
ifneq ($(wildcard $(LIBFT_MK)/functions.mk),)
include $(LIBFT_MK)/functions.mk
include $(LIBFT_MK)/symbols.mk
else
log_note  = @printf "$(BOLD_MAGENTA)%s$(RESET)\n" "$(1)"
log_info  = @printf "$(BOLD_GREEN)%s$(RESET)\n" "$(1)"
log_ok    = @printf "  $(BRIGHT_CYAN)✓$(RESET) %s\n" "$(1)"
log_warn  = @printf "$(BRIGHT_YELLOW)⚠ %s$(RESET)\n" "$(1)"
log_error = @printf "$(BRIGHT_RED)✗ %s$(RESET)\n" "$(1)"
endif

# ── Default goal ────────────────────────────────────────────────────────────
# `make` with no arguments prints the help instead of building. That is a
# deliberate trade: discovering ~60 targets used to mean reading 900 lines of
# Makefile, and the cost is that anything wanting a build must now say `all`.
# Every call site in this repo was updated in the same commit; if you are
# adding one, `make all` is the spelling.
.DEFAULT_GOAL := help

# ── Self-documenting help ───────────────────────────────────────────────────
# The help text IS this file. Three annotations, rendered by mk/help.awk:
#
#   ##@ Section            a heading, in the order they appear here
#   target: deps  ## text  a target and what it does
#   ##! NAME=value  text   a variable you can set on the command line
#
# Generated rather than written out, because a hand-written list drifts: a
# target with no `##` simply does not appear, so documenting one is part of
# adding it. `make help-targets` prints the undocumented ones, which is the
# check that keeps that honest.
HELP_AWK := mk/help.awk

##@ Configuration
##! MODE=release  Build config: debug (default) | release | relwithdebinfo
##! OPT=1  Older spelling of MODE=release; still supported
##! SAFE=1  libc malloc (ASan can see it). SAFE=0 uses ft_malloc
##! CC=clang  Compiler to build with (default cc)
##! ROUNDS=7  Benchmark repetitions for `make bench`
##! BENCH=micro  Benchmark set: micro or full
##! SUITE_ARGS="redir pipe"  Restrict `make docker-suite` to these categories
##! ORACLE_PREFIX=~/bash-5.3.9  Where `make oracle` builds the pinned bash
##! PREFIX=~/.local  Install root for `make user-install`
##! EXTRA_CFLAGS=...  Appended to CFLAGS (e.g. -DFOO); never replaces them
##! EXTRA_LDFLAGS=...  Appended to LDFLAGS (e.g. -static)
##! NO_COLOR=1  Drop every escape sequence from make's own output

help: ## Show this help (default target)
	@printf "\n$(BOLD_WHITE)hellish$(RESET) — a POSIX shell in C\n"
	@printf "$(DIM)usage:$(RESET)  make $(BOLD_GREEN)<target>$(RESET) [$(BOLD_CYAN)VAR=value$(RESET) ...]\n"
	@printf "$(DIM)build:$(RESET)  make $(BOLD_GREEN)all$(RESET)   $(DIM)(plain \`make\` shows this page)$(RESET)\n"
	@awk -v head="$(BOLD_YELLOW)" -v tgt="$(BOLD_GREEN)" -v var="$(BOLD_CYAN)" \
		-v reset="$(RESET)" -f $(HELP_AWK) $(firstword $(MAKEFILE_LIST))
	@printf "\n$(DIM)undocumented targets: make help-targets  ·  platforms: wiki/platforms.md$(RESET)\n\n"

help-targets: ## List targets missing a ## description (should print none)
	@undoc=$$(awk '/^[a-zA-Z0-9_.-]+:/ && !/^\t/ && !/## / { t = $$0; sub(/:.*/, "", t); print t }' \
		$(firstword $(MAKEFILE_LIST)) | grep -vxF -e .PHONY -e safe_banner | sort -u); \
	if [ -z "$$undoc" ]; then \
		printf "  $(BRIGHT_CYAN)✓$(RESET) every target is documented\n"; \
	else \
		printf "$(BRIGHT_YELLOW)⚠ undocumented targets:$(RESET)\n"; \
		printf "    %s\n" $$undoc; \
		exit 1; \
	fi

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

# Optimized-but-debuggable. -O2 rather than -O3 and no LTO, both so the code
# a debugger shows you still resembles the code you wrote. -DNDEBUG matches
# release, because a bug that only appears with assertions compiled out is
# precisely the kind this configuration is for.
RELDEBFLAGS := -O2 -g -DNDEBUG

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

# ── Build configurations ────────────────────────────────────────────────────
#
# Three, named, chosen with MODE=. They exist because "make it smaller" and
# "keep it debuggable" are different jobs and a single set of flags cannot do
# both -- and because the answer to a Release-only bug must not be "rebuild
# with symbols and hope it still reproduces".
#
#   MODE=debug           -O0 -g3 + ASan/LSan.  The DEFAULT, and the one you
#                        develop against. Big on purpose: the sanitizer
#                        runtime and full DWARF are most of it. Never
#                        shipped.
#   MODE=release         -O3 + LTO, -DNDEBUG, NO -g, no sanitizer. What is
#                        installed and what the release artifacts are built
#                        from. ~620 KB.
#   MODE=relwithdebinfo  -O2 -g, -DNDEBUG, no sanitizer, no LTO. Optimized
#                        code you can still put a debugger on -- for the
#                        bugs that only show up with optimization. LTO is
#                        deliberately off here: it is what turns a stack
#                        trace into a list of inlined addresses.
#
# Nothing is stripped after the fact. Release simply never compiles -g in,
# which is why `strip` finds only ~1 KB left to remove: the handful of DWARF
# stubs the C runtime brings with it, not our code.
#
# OPT=1 is the older spelling of MODE=release and stays working -- CI, the
# install targets, the docker build and `make bench` all pass it, and those
# are exactly the callers you do not want to break to rename a flag.
ifdef OPT
MODE ?= release
endif
MODE ?= debug

CPPFLAGS := $(INCLUDES)

ifeq ($(MODE),release)
CFLAGS   := $(CFLAGS_BASE) $(OPTFLAGS_COMMON) \
            $(if $(CC_IS_GCC),$(OPTFLAGS_GCC),) \
            $(if $(CC_IS_CLANG),$(OPTFLAGS_CLANG),) \
            $(LTO_CFLAGS)
LDFLAGS  := $(LDFLAGS_BASE) $(LTO_LDFLAGS)
else ifeq ($(MODE),relwithdebinfo)
CFLAGS   := $(CFLAGS_BASE) $(RELDEBFLAGS) \
            $(if $(CC_IS_GCC),$(OPTFLAGS_GCC),) \
            $(if $(CC_IS_CLANG),$(OPTFLAGS_CLANG),)
LDFLAGS  := $(LDFLAGS_BASE)
else ifeq ($(MODE),debug)
CFLAGS   := $(CFLAGS_BASE) $(DEBFLAGS) $(SANFLAGS)
LDFLAGS  := $(LDFLAGS_BASE) $(SANFLAGS)
else
$(error MODE must be debug, release or relwithdebinfo -- got '$(MODE)')
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
#
# Keyed off MODE, not OPT. It used to read `ifdef OPT`, which meant OPT=1 and
# MODE=release -- the same configuration under two names -- disagreed about the
# allocator and so produced different binaries. Both optimized modes exercise
# ft_malloc; debug stays on libc so AddressSanitizer can still see every
# allocation, which is the entire reason the debug build exists.
ifeq ($(MODE),debug)
SAFE ?= 1
else
SAFE ?= 0
endif
ifeq ($(SAFE),0)
SAFE_TAG := ft
# ft_malloc's leak oracle exists only on this backend, so say so at COMPILE
# time. alloc_stats.c used to work it out at LINK time instead, with a weak
# undefined reference and a -Wl,-u to force the archive member -- an ELF-only
# trick. Mach-O reads __attribute__((weak)) on a declaration as a weak
# DEFINITION rather than a weak import, so Apple's linker demanded a body and
# the macOS arm64 build died on "Undefined symbols: _malloc_live_bytes".
# A plain strong reference pulls the archive member by itself, which is why
# the -u is gone with the weak ref rather than kept alongside this.
#
# CFLAGS, not CFLAGS_BASE: CFLAGS is expanded with := about forty lines above
# this block, so appending to the base here would land after the expansion and
# do nothing at all.
CFLAGS += -DHAVE_ALLOC_ORACLE
else
SAFE_TAG := libc
endif

# Directories. The object tree is keyed on the build mode AND the allocator
# backend, because make rebuilds on a changed prerequisite and never on a
# changed flag: a tree filled by MODE=debug and then reused by MODE=release
# hands the linker ASan-instrumented objects while the link line carries no
# -fsanitize at all, and the build dies on "undefined reference to
# `__asan_report_load4'". SAFE is in the key too -- it decides
# -DHAVE_ALLOC_ORACLE, a compile-time define. This used to key on `ifdef OPT`,
# which covered only the OPT benchmark build and left MODE=release and
# MODE=relwithdebinfo sharing the debug tree. The binary path stays shared and
# is relinked per mode, so `make bench` still times a true optimized build.
OBJ_DIR := build/obj-$(MODE)-$(SAFE_TAG)
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

##@ Build
all: safe_banner $(BIN_DIR)/$(BAPTIZE_SHELL)  ## Build the shell → build/bin/hellish

# Announce the active allocator AND the build mode before building, so
# neither is ever a surprise.
#
# The MODE line is not decoration. build/bin/hellish is a single shared path
# that every mode relinks (deliberately -- see OBJ_DIR above), and `make test`
# rebuilds at the DEFAULT mode, which is debug. So
#
#     make all OPT=1      # release binary
#     make test           # ...silently replaced it with a debug+ASan one
#
# left you benchmarking, shipping or `make my_shell`-installing a 7.4MB ASan
# build while believing it was the 619KB release one. Nothing said so. The
# stamp below remembers what the binary currently IS, and the link rule says
# out loud when that changes.
safe_banner:
	@printf "  \033[1;36m▸\033[0m \033[1;37mMODE=%s\033[0m \033[90m→ %s\033[0m\n" \
		"$(MODE)" "$(BIN_DIR)/$(BAPTIZE_SHELL)" >&2
	@if [ "$(SAFE)" = "0" ]; then \
		printf "\n  \033[1;31m⚠  SAFE=0\033[0m \033[1;37m— custom ft_malloc heap (faster, UNSAFE).\033[0m\n" >&2; \
		printf "  \033[90mPass SAFE=1 for the libc allocator. Stability is on you.\033[0m\n\n" >&2; \
	else \
		printf "\n  \033[1;32m✓  SAFE=1\033[0m \033[1;37m— libc malloc/free.\033[0m \033[90m(OPT build defaults to SAFE=0 ft_malloc)\033[0m\n\n" >&2; \
	fi

# Link the final binary.
#
# MODE_STAMP records which configuration the binary at this shared path was
# last linked from. If the mode changed under you, say so -- that is the whole
# defence against the `make all OPT=1 && make test` trap described above.
MODE_STAMP := $(BIN_DIR)/.mode

$(BIN_DIR)/$(BAPTIZE_SHELL): $(LIBFT_A) $(OBJS)
	@mkdir -p $(BIN_DIR)
	@prev=$$(cat $(MODE_STAMP) 2>/dev/null || echo ""); \
	if [ -n "$$prev" ] && [ "$$prev" != "$(MODE)-$(SAFE_TAG)" ]; then \
		printf "  \033[1;33m!\033[0m \033[1;37m%s was %s, now relinked as %s\033[0m\n" \
			"$(BIN_DIR)/$(BAPTIZE_SHELL)" "$$prev" "$(MODE)-$(SAFE_TAG)" >&2; \
	fi
	$(CC) $(CFLAGS) $(OBJS) $(LIBFT_A) $(LDFLAGS) $(LDLIBS) -o $@
	@printf '%s\n' "$(MODE)-$(SAFE_TAG)" > $(MODE_STAMP)

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

clean:  ## Remove object files, keep the binary
	@printf "\n  \033[1;33m⚠\033[0m \033[1;37mCleaning build artifacts\033[0m" >&2
	@rm -rf $(OBJ_DIR)
	@printf "\r\033[K  \033[1;32m✓\033[0m \033[37mBuild artifacts cleaned\033[0m\n\n" >&2

# SAFE is forwarded explicitly to the libft sub-make below. Only a SAFE= typed
# on the command line propagates on its own (via MAKEFLAGS); the per-mode
# default set above does not, so a plain `make fclean` used to reach libft with
# SAFE unset -- landing in its SAFE!=1 branch, running the LTO capability probe
# and printing its warning just to delete files.
fclean: clean  ## Remove objects, the binary and libft's build trees
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
re:  ## fclean + rebuild (OPT/SAFE propagate)
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

##@ Test
ZSH_ORACLE_PREFIX ?= $(HOME)/zsh-5.9

# The zsh dialect's oracle, same arrangement as `oracle` below: the flag
# semantics are not inferable from our source (see tests/build_zsh_oracle.sh),
# so the tests diff against a real zsh or they skip and say so.
zsh-oracle:  ## Build the zsh 5.9 the zsh-dialect tests are defined against
	@/bin/bash tests/build_zsh_oracle.sh "$(ZSH_ORACLE_PREFIX)"

oracle:  ## Build the PINNED bash 5.3.9 the suite is defined against
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
test:  ## Golden suite: ~3800 cases diffed against bash --posix
	@rm -f $(BIN_DIR)/$(BAPTIZE_SHELL)
	@$(MAKE) --no-print-directory all
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mRunning tests\033[0m\n\n" >&2
	@(cd $(TEST_DIR); /bin/bash $(BIN_TEST))

# The same suite against the RELEASE build.
#
# `test` above builds at the DEFAULT mode, which is debug+ASan, so until this
# existed nothing ever ran the golden cases against what we actually ship. A
# heap bug that ASan happened to render benign passed 3790/3790 while the
# release binary segfaulted on `V=1 cmd` -- one of the most common constructs
# there is, with nine cases in tests/var that all "passed". Optimisation and
# the sanitizer disagree about uninitialised stack, so one run cannot stand
# in for the other.
test-release:  ## Golden suite against the RELEASE build (not debug+ASan)
	@rm -f $(BIN_DIR)/$(BAPTIZE_SHELL)
	@$(MAKE) --no-print-directory all OPT=1 SAFE=1
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mRunning tests (RELEASE build)\033[0m\n\n" >&2
	@(cd $(TEST_DIR); /bin/bash $(BIN_TEST))

# Official speed verdict vs `bash --posix`. Always benchmarks the OPT build
# (timing the default ASan/debug build would be meaningless). Override rounds /
# scope:  make bench ROUNDS=7        make bench BENCH=micro
##@ Benchmarks & conformance
bench:  ## Speed vs bash --posix (always rebuilds OPT=1)
	@rm -f $(BIN_DIR)/$(BAPTIZE_SHELL)
	@$(MAKE) --no-print-directory OPT=1 all
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mBenchmarking hellish vs bash --posix\033[0m\n\n" >&2
	@(cd $(TEST_DIR); ROUNDS=$(ROUNDS) TIMEOUT_S=$(TIMEOUT_S) /bin/bash benchmark $(BENCH))

# tests/born2root is a submodule of someone else's scripts; its C files are
# not 42-norm and not ours to reformat, so the tests/ sweep leaves it out.
norm:  ## 42 norminette over src/ incs/ tests/ (reports only, always exits 0)
	@printf "\n  \033[1;36m▸\033[0m Running norminette" >&2; \
	output="$$( \
	    norminette src incs $$(find tests -name '*.[ch]' -not -path '*/born2root/*') \
	        2>&1 | grep -v 'OK!' | grep -v 'US' \
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
# optimized AND safe (OPT=1 SAFE=1 -> libc allocator) by default.
#
# It also seeds ~/.hellishrc, which it used not to: the seeding lived inside
# user-install.sh, so this route installed a binary and stopped, and the
# first thing you met was a shell with no config -- no EDITOR, no aliases,
# no PS1 (issue #51). Both routes now call tools/seed_hellishrc.sh, which
# never touches an rc you already have. It runs BEFORE chsh: the config
# should be in place before anything can log you into the new shell. You may force
# the custom heap with `make my_shell SAFE=0`, but then stability is on you.
#
# Installing and registering are tools/register_shell.sh, in THIS repo. They
# used to be a raw `sudo install` here plus vendor/scripts/register_shell.sh,
# fourteen unchecked lines in a submodule -- which meant the most dangerous
# step in the whole build (rewriting your passwd entry) was the one step with
# no preflight, no smoke test and no test coverage. It failed in a clean
# container on a bare `chsh` that prompts for a password make cannot answer,
# and it would happily make a binary that does not run your login shell. See
# the header of tools/register_shell.sh and tests/register_shell_test.py.
#
# --preflight runs FIRST, before the rebuild: "chsh is not installed" is worth
# knowing before three minutes of compiling, not after.
# Which binary gets installed. STATIC=1 takes the container-built static one
# from dist/ instead of compiling here -- the "build it in docker, then run it
# on my machine" flow. Recursively expanded (`=`, not `:=`) because STATIC_OUT
# is defined further down the file.
MY_SHELL_BIN = $(if $(filter 1,$(STATIC)),$(STATIC_OUT),$(BIN_DIR)/$(BAPTIZE_SHELL))

# Install a PUBLISHED release instead of this working tree:
#
#   make my_shell VERSION=2.7.2
#
# The point is bug reports. A user's problem lives in the binary they already
# have, and issue #76 is the extreme case -- the bug is IN THE UPDATER, so it
# cannot be reproduced from HEAD at all: you have to install the old one and
# press the button. With this, `make my_shell VERSION=2.7.2` puts you exactly
# where the reporter was, on your own machine.
#
# It skips the build entirely and downloads the release asset (checksum
# verified, and proven to run before anything installs it -- see
# tools/fetch_release.sh). VERSION= and STATIC=1 are mutually exclusive:
# STATIC builds a binary here, VERSION fetches one that already exists.
VERSION ?=
RELEASE_BIN := build/bin/hellish-release
ifneq ($(VERSION),)
MY_SHELL_BIN = $(RELEASE_BIN)
endif

##@ Build info
# Print the configuration a given MODE actually resolves to, without building
# anything. This is how you check what you are about to ship -- and what the
# build-config test asserts against, so the three modes cannot quietly drift
# into each other.
#
#   make flags                     the default (debug)
#   make flags MODE=release        what the release artifacts are built with
flags:  ## Show the compiler/linker flags for this MODE
	@printf 'MODE=%s\n' '$(MODE)'
	@printf 'CFLAGS=%s\n' '$(CFLAGS)'
	@printf 'LDFLAGS=%s\n' '$(LDFLAGS)'
	@printf 'LDLIBS=%s\n' '$(LDLIBS)'
	@printf 'OBJ_DIR=%s\n' '$(OBJ_DIR)'

##@ Install
my_shell:  ## sudo-install to /usr/bin and register as a login shell (VERSION=2.7.2 for a release)
	@if [ -n "$(VERSION)" ] && [ "$(STATIC)" = "1" ]; then \
		echo "make: VERSION= and STATIC=1 are mutually exclusive --"; \
		echo "  VERSION fetches a published binary, STATIC builds one here."; \
		exit 1; \
	fi
	@./tools/register_shell.sh --preflight
	@if [ -n "$(VERSION)" ]; then \
		mkdir -p $(BIN_DIR); \
		./tools/fetch_release.sh "$(VERSION)" "$(RELEASE_BIN)" >/dev/null; \
	elif [ "$(STATIC)" = "1" ]; then \
		$(MAKE) --no-print-directory static-verify; \
	else \
		$(MAKE) --no-print-directory re OPT=1 \
			SAFE=$(if $(filter command line,$(origin SAFE)),$(SAFE),1); \
	fi
	@echo "Seeding your config..."
	@./tools/seed_hellishrc.sh
	@./tools/register_shell.sh --bin $(MY_SHELL_BIN) --dest /usr/bin/hellish
	@echo "Done. Log out and log back in to use hellish as your default shell."
	@echo 'if impatient, replace the shell in THIS terminal, no relog needed:'
	@echo '    exec /usr/bin/hellish --login'
	@if [ -n "$(VERSION)" ]; then \
		printf '\n  \033[1;33m!\033[0m installed the PUBLISHED v%s, not this working tree.\n' \
			"$(VERSION)"; \
		printf '    to reproduce a report from here:  hellish -c "update --now"\n'; \
		printf '    to go back to your build:         make my-shell-uninstall && make my_shell\n\n'; \
	fi

# Undo my_shell completely: login shell restored, binary gone, /etc/shells
# entry gone. The config is KEPT -- ~/.hellishrc is your own work and
# reinstalling to test something is not a reason to lose it.
#
# The login shell is restored BEFORE the binary is deleted, and a failed chsh
# aborts without deleting anything: the reverse order can leave an account
# whose login shell does not exist, which locks you out of ssh and every tty.
my-shell-uninstall:  ## Undo make my_shell (keeps your ~/.hellishrc)
	@./tools/register_shell.sh --uninstall --dest /usr/bin/hellish

# ...and take the config too, for a genuinely clean slate. This is the one to
# pair with VERSION= when reproducing a report: a stale ~/.cache/hellish
# remembers which release it already told you about, so a reinstall of an old
# version can start out believing it is current.
my-shell-purge:  ## Undo make my_shell AND delete ~/.hellishrc + caches
	@./tools/register_shell.sh --purge --dest /usr/bin/hellish

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
user-install:  ## Install to ~/.local/bin with an rc hook — no sudo needed
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

user-uninstall:  ## Remove the rc hook and the binary
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

##@ Ship
static:  ## Static musl binary via docker → dist/hellish-linux-<arch>
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
static-verify: static  ## ... and prove it runs on THIS host
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
docker: static-verify  ## Build the release image from the verified static binary
	@printf "  \033[1;37mInstall it as your login shell with:\033[0m\n" >&2
	@printf "      \033[1;36mmake my_shell STATIC=1\033[0m\n\n" >&2

# Hermetic golden suite: build the shell AND the pinned bash 5.3.9 oracle in
# one image, then diff them there. This is the run that cannot be wrong because
# of the host -- both sides of the comparison come from the image. Use it when a
# local `make test` disagrees with CI, or on any machine whose bash is not 5.3.
#   make docker-suite                      # everything
#   make docker-suite SUITE_ARGS="redir pipe"
docker-suite:  ## The golden suite, hermetic: shell AND oracle from the image
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
docker-build:  ## Build the per-distro images used by docker-test
	docker compose build
docker-test:  ## Build + smoke from source on every supported distro
	@chmod +x docker/test.sh docker/smoke.sh && docker/test.sh
docker-alpine:  ## Interactive hellish in an Alpine container
	docker compose run --rm alpine
docker-debian:  ## Interactive hellish in a Debian container
	docker compose run --rm debian
docker-ubuntu:  ## Interactive hellish in an Ubuntu container
	docker compose run --rm ubuntu
docker-ubuntu2204:  ## Interactive hellish in Ubuntu 22.04 (the WSL default)
	docker compose run --rm ubuntu2204
docker-arch:  ## Interactive hellish in an Arch container
	docker compose run --rm arch
docker-fedora:  ## Interactive hellish in a Fedora container
	docker compose run --rm fedora
docker-rocky:  ## Interactive hellish in a Rocky Linux container
	docker compose run --rm rocky
docker-opensuse:  ## Interactive hellish in an openSUSE container
	docker compose run --rm opensuse
docker-void:  ## Interactive hellish in a Void Linux container
	docker compose run --rm void
docker-clean:  ## Remove the images and volumes these targets create
	docker compose down --rmi local 2>/dev/null || true

# The portability workout on its own, against whatever binary is built. Same
# script the distro containers and every Platforms CI rung run, so a failure
# reads the same everywhere.
smoke: all  ## 40-check portability workout against your build
	@chmod +x docker/smoke.sh && docker/smoke.sh $(BIN_DIR)/$(BAPTIZE_SHELL)

# Cross-shell speed matrix. Build hellish + a zoo of other shells (bash, dash,
# zsh, mksh, ksh, yash, busybox ash, fish) in ONE self-contained image, then
# race them all on a portable POSIX workload set and print who is fastest and
# where hellish lands. The host needs none of those shells installed -- that is
# the whole point of doing it in docker. See tests/agnostic_bench.sh.
# Override rounds/timeout:  make agnostic-bench ROUNDS=7 TIMEOUT_S=60
agnostic-bench:  ## Cross-shell speed matrix vs 8 shells, in docker
	docker build -f docker/Dockerfile.agnostic -t hellish:agnostic .
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mRacing hellish against every shell we could install\033[0m\n\n" >&2
	@mkdir -p bench/.artifacts
	docker run --rm -e ROUNDS=$(ROUNDS) -e TIMEOUT_S=$(TIMEOUT_S) hellish:agnostic \
		| tee bench/.artifacts/agnostic-matrix.txt
	@python3 bench/lib/gen_matrix_chart.py

# Build hellish + zsh in one image and diff the zsh-style two-argument
# `cd old new` extension against real zsh (the bash suite can't cover it).
cd-zsh-test:  ## docker: the zsh-style `cd old new` extension vs real zsh
	docker build -f docker/Dockerfile.zsh -t hellish:zsh .
	docker run --rm hellish:zsh

# `make my_shell` and the update that follows it, on a machine shaped like the
# one in issue #76: Ubuntu, a NON-ROOT human with a password-protected sudo,
# and hellish in /usr/bin owned by root.
#
# It runs the REAL my_shell target -- chsh, /etc/shells, the passwd entry --
# which is exactly why it cannot run on a developer host: a test may not
# rewrite your login shell. The permission shape alone is covered without
# root, and in CI, by tests/update_sudo_fail_test.py.
#
# The release is a local fake, so this proves the mechanism rather than
# whatever github is serving today, and needs no network.
my-shell-test:  ## docker: make my_shell, then the update button (issue #76)
	docker build -f docker/Dockerfile.my-shell -t hellish:my-shell .
	docker run --rm hellish:my-shell

# hellish as a real LOGIN SHELL behind a real sshd. After `make my_shell`,
# sshd execs hellish for every remote operation -- ssh-command, scp, sftp,
# rsync and git all run `$SHELL -c ...` -- and those protocols die on a single
# stray byte of stdout. The golden suite cannot see any of it: it runs
# `hellish -c` with no sshd, no chsh and no pipe. Every case is diffed against
# bash as the login shell, so nothing can encode a hellish bug as "expected".
ssh-shell-test:  ## docker: hellish as a login shell (ssh/scp/rsync/git) vs bash
	docker build -f docker/Dockerfile.sshd -t hellish:sshd .
	docker run --rm hellish:sshd

# install.sh end-to-end, both privilege worlds, in a container -- see
# docker/Dockerfile.installer for who lives there and why. Needs the static
# binary (it plays the role of the release asset) and the hellishrc_plugins
# working copy: PLUGINS_DIR=path/to/checkout, or it clones from GitHub.
PLUGINS_DIR ?=
installer-test: static  ## docker: curl|sh installer, sudo AND no-sudo + plugin picks
	@if [ -z "$(PLUGINS_DIR)" ]; then \
		rm -rf build/hellishrc_plugins; \
		git clone -q --depth 1 https://github.com/Univers42/hellishrc_plugins \
			build/hellishrc_plugins; \
	fi
	docker build -f docker/Dockerfile.installer \
		--build-context plugins=$(if $(PLUGINS_DIR),$(PLUGINS_DIR),build/hellishrc_plugins) \
		-t hellish:installer .
	docker run --rm hellish:installer

# The same report `make my_shell` prints at the end, on demand. Answers the
# two questions behind every "the update did nothing" report: WHICH hellish
# does PATH actually reach, and can its directory be written to.
#
# Reports; never fails the target. The findings are things about the MACHINE
# (a second copy on PATH, a root-owned install dir), not build failures, and a
# red `make doctor` would train people to ignore it. The script's own exit
# status still means something for anything that wants to check it.
doctor:  ## Which hellish is on PATH, and will `update` be able to install?
	@./tools/register_shell.sh --doctor || true

# Host-side check (no docker): `hellish --posix` must match `bash --posix` on
# the cd cases the zsh extension would otherwise change, while normal mode keeps
# the extension. Builds first so the binary is current.
cd-posix-test: all  ## --posix gates that cd extension off
	@chmod +x $(TEST_DIR)/cd_posix_compare.sh
	@HELLISH=$(BIN_DIR)/$(BAPTIZE_SHELL) bash $(TEST_DIR)/cd_posix_compare.sh

# Interactive multi-line history regression test (real pty): entries keep
# their multi-line text in `history` and the file, and up-arrow recall of
# loops/here-docs re-executes with bash-cmdhist semantics instead of the
# broken space-joined flattening. See tests/hist_multiline_test.py.
hist-test: all  ## pty: cmdhist multiline history joining
	@python3 $(TEST_DIR)/hist_multiline_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# The `history` builtin's OPTIONS, in a live session (issue #42). The golden
# category issue42_history_opts covers what each one does under `-c`; this
# covers the shape the bug actually had -- an interactive shell, a populated
# history file, and PROMPT_COMMAND='history -a' dumping the whole list before
# every prompt. Run it after touching builtin_history*.c.
history-opts-test: all  ## pty: the history builtin's options (#42)
	@python3 $(TEST_DIR)/history_opts_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Every multi-line construct, in BOTH history modes, diffed against the pinned
# bash 5.3.9 driving the SAME keystrokes in the same pty (issue #32):
# backslash-continuation, if/elif/else, for, while, until, case, function
# definitions, brace groups, subshells, unterminated ' " and `, $( ), $(( )),
# here-docs, and a trailing | && ||. Checks the listing AND what the up-arrow
# puts back, because those are two different code paths in hellish.
# Needs the oracle: `make oracle`.
history-matrix-test: all  ## pty: 26 multi-line constructs × 2 modes vs pinned bash
	@python3 $(TEST_DIR)/history_multiline_matrix.py \
		$(BIN_DIR)/$(BAPTIZE_SHELL)

# The git dirty star must not outlive the state it describes: a `git status`
# that took a second armed a 30-second TTL, and the prompt then asserted
# "dirty" for half a minute after a `git checkout` in the same shell had made
# the tree clean. Makes the slow scan deterministic with a `git` shim rather
# than needing a big repo, so it reproduces on any machine.
git-star-test: all  ## pty: the git dirty star never outlives the state it describes
	@python3 $(TEST_DIR)/git_star_freshness_test.py \
		$(BIN_DIR)/$(BAPTIZE_SHELL)

# ── Every pty/regression test in tests/*.py, by DISCOVERY ────────────────────
# Not a list: a list drifts, and this one had. completion_posix_test.py lived
# in tests/ with no target and no CI job, so the POSIX command-search fix it
# guards ran nowhere from the day it landed. tests/pty_suite.sh globs the
# directory instead, so a new regression test is covered the moment it exists.
# This is what CI runs; the individual targets above stay for working on one.
##@ Interactive (pty) gates
pty-test: all  ## EVERY tests/*.py regression test, by discovery — what CI runs
	@chmod +x $(TEST_DIR)/pty_suite.sh && $(TEST_DIR)/pty_suite.sh

# The prompt WIDTH model, linked directly. Not a pty case and not by choice:
# the width is what the line editor uses to place the cursor, it is never
# printed, and no shell command reveals it -- so the only shell-level
# observable is where a line wraps. The pty case that inferred it that way
# PASSED against a binary with the bug still in it, which is the one outcome
# a test may never have. See tests/prompt_width_test.c.
prompt-width-test: all  ## unit: visible_width_cstr (CSI, OSC, guards, wide glyphs)
	@chmod +x $(TEST_DIR)/prompt_width_test.sh && $(TEST_DIR)/prompt_width_test.sh

# Twelve real third-party plugins, each with a declared expectation. Runs
# against BOTH builds because that is how it earns its keep: the git-prompt.sh
# segfault existed only in release while the golden suite passed 3790/3790 in
# debug, and the 18 KB alias leak is invisible to ASan (still-reachable) and
# only shows on the ft_malloc oracle. A corpus that ran one build would have
# missed one of them.
#
# Vendors nothing and skips cleanly offline, so a CI box with no network
# still passes rather than pretending to have checked.
plugin-corpus: ## Real plugins vs release AND ASan — the compatibility matrix
	@rm -f $(BIN_DIR)/$(BAPTIZE_SHELL)
	@$(MAKE) --no-print-directory MODE=release all
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mcorpus: release\033[0m\n" >&2
	@python3 $(TEST_DIR)/plugin_corpus_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mframework: release\033[0m\n" >&2
	@python3 $(TEST_DIR)/hxp_framework_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)
	@rm -f $(BIN_DIR)/$(BAPTIZE_SHELL)
	@$(MAKE) --no-print-directory all
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mcorpus: debug + ASan\033[0m\n" >&2
	@ASAN_OPTIONS=detect_leaks=1 python3 $(TEST_DIR)/plugin_corpus_test.py \
		$(BIN_DIR)/$(BAPTIZE_SHELL)
	@printf "\n  \033[1;36m▸\033[0m \033[1;37mframework: debug + ASan\033[0m\n" >&2
	@ASAN_OPTIONS=detect_leaks=1 python3 $(TEST_DIR)/hxp_framework_test.py \
		$(BIN_DIR)/$(BAPTIZE_SHELL)

hxp-test: all  ## The plugin framework (hxp) end to end: install, load, use every plugin
	@python3 $(TEST_DIR)/hxp_framework_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Non-ASCII in the PROMPT (the shortened cwd) and in TYPED INPUT (a wrapping
# line that starts with a two-byte, one-column character, plus an edit made
# at its start -- the shape reported in issue #2). Both render the pty output
# through a terminal model and compare it against what should be on screen.
widechar-test: all  ## pty: non-ASCII in the prompt and in a wrapping typed line
	@python3 $(TEST_DIR)/widechar_prompt_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)
	@python3 $(TEST_DIR)/widechar_input_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Every libreadline entry point hellish uses, driven through a real pty. The
# golden suite cannot reach ANY of it -- every category runs `hellish -c`,
# which never enters the readline path -- so this is the only gate protecting
# completion, history recall and vi/emacs switching. Required before touching
# the readline linkage (see backlog: dlopen readline). tests/readline_paths_test.py
readline-test: all  ## pty: every libreadline entry point — run before touching linkage
	@python3 $(TEST_DIR)/readline_paths_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Prompt-animation vs paste regression (real pty): the idle repaint must
# freeze whenever the cursor may have left the input's first screen row
# (pasted newline, wrapped or multibyte input) instead of climbing a
# mis-computed row count and erasing the paste or the scrollback. Also
# proves the animation still runs on plain input. See tests/anim_paste_test.py.
anim-test: all  ## pty: the prompt animation never clobbers pasted input
	@python3 $(TEST_DIR)/anim_paste_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Prompt git segment must never block on `git status` (real pty): cd into
# a repo where the scan takes seconds shows the next prompt immediately
# (the check finishes in the background and the star arrives a render
# late), TTL refreshes stay non-blocking, and normal-speed repos keep
# their exact synchronous star. See tests/git_prompt_stall_test.py.
git-prompt-test: all  ## pty: the prompt's git check never blocks a render
	@python3 $(TEST_DIR)/git_prompt_stall_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Background jobs and the controlling terminal. POSIX only redirects an
# async list's stdin to /dev/null when job control is DISABLED; doing it
# unconditionally broke `top &` interactively ("top: failed tty get")
# instead of letting it stop on SIGTTOU the way bash does. Compares
# against bash directly, so it also notices if bash changes its mind.
bg-tty-test: all  ## pty: background jobs and terminal ownership
	@python3 $(TEST_DIR)/bg_tty_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# A virtualenv prompt surviving its own deactivate. venv's activate saves
# ${PS1:-} and restores it only `if [ -n "$$_OLD_VIRTUAL_PS1" ]`, so a shell
# with no default PS1 saved "" and stayed "(venv) " forever. Runs the PS1
# handshake verbatim, then the real python3 -m venv round trip. (#39)
venv-prompt-test: all  ## pty: the virtualenv segment, incl. a real python3 -m venv
	@python3 $(TEST_DIR)/venv_prompt_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Tab completion, built SAFE=0 ON PURPOSE. readline frees every match it is
# handed with libc free(), so a match allocated by ft_malloc is a cross-heap
# free -- and that is invisible on SAFE=1, where xmalloc IS libc malloc and
# there is no mismatch to find. Only a SAFE=0 binary can fail this gate, so
# it builds one (ASan there reports "bad-free ... not malloc()-ed"). It also
# covers the PATH scan that stopped at the first match per directory. (#40)
completion-test:  ## pty: tab completion on a SAFE=0 build (cross-heap frees)
	@$(MAKE) --no-print-directory all SAFE=0
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
completion-posix-test: all  ## pty: what TAB is ALLOWED to offer (POSIX command search)
	@python3 $(TEST_DIR)/completion_posix_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# PS1 rendering, and the login chain that exposed it. Because hellish sets
# BASH_VERSION, Debian/Ubuntu's stock ~/.profile sources ~/.bashrc and hands
# us bash's default PS1 -- which the renderer then printed as visible text
# (":+()}\033[01;32m...") because \nnn was not an escape and ${v:+w} was read
# as a bare name. Renders that exact PS1 in a pty and checks the bytes.
ps1-render-test: all  ## pty: a bare-name PS1 renders byte-for-byte
	@python3 $(TEST_DIR)/ps1_render_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Canary (NOT a regression gate for a specific fix): drives the shell the way
# the field reports of intermittent prompt corruption describe, and asserts no
# escape sequence reached the screen without its ESC[ and no UTF-8 character
# was cut. It has never reproduced that corruption -- read its docstring
# before trusting a pass.
prompt-integrity-test: all  ## pty: prompt bytes survive a short write
	@python3 $(TEST_DIR)/prompt_shortwrite_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# The pending-update badge. The loud notice is one-shot by design; this is
# the quiet marker that persists while an update is actually waiting, so a
# user who missed the notice still finds out. Also covers the \U escape.
update-badge-test: all  ## pty: the lazy update header and its \U escape
	@python3 $(TEST_DIR)/update_badge_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Release/update configuration, checked offline: the GitHub slug baked into
# version.h must match the repository we actually push to, every
# distribution channel (install.sh, npm, Dockerfile, docs) must agree with
# it, and npm's version must track HELLISH_VERSION. No runtime test can
# reach any of this -- a wrong slug only shows up as a 404 at update time.
update-config-test:  ## Offline gate: version, GitHub slug, install.sh, npm and docs agree
	@bash $(TEST_DIR)/update_config_check.sh

# The `help` builtin. The load-bearing check is that EVERY builtin in the
# dispatch table has a help entry -- documentation rots by omission, and
# deriving the expected set from hash_builtins_dispatch.c makes that
# impossible to do quietly.
help-test: all  ## The help builtin — every dispatch-table entry must have one
	@bash $(TEST_DIR)/help_test.sh $(BIN_DIR)/$(BAPTIZE_SHELL)

# born2root is the acceptance corpus for the bash dialect: a real project of
# ~145 scripts (tests/born2root, a submodule) that builds a Debian VM with
# hellish as the guest's shell. Everything short of building the VM: every
# script parses like bash, and its self-contained unit tests print the same.
born2root-test: all  ## born2root corpus: every script parses like bash, its unit tests print the same
	@bash $(TEST_DIR)/born2root_check.sh

# The builtins page of the docs site IS `help` output (help-test above is
# what stops it drifting from the dispatch table). Regenerate + commit after
# adding or changing a builtin; the site build needs no compiler this way.
docs-builtins: all  ## Regenerate wiki/builtins/index.md from the help builtin
	@python3 tools/gen_builtins_md.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# The parse arena's rare states (chunk boundaries, registry-full heap
# fallback) need hundreds of MB to reach at production chunk sizes — the
# issue #94 double free lived there for months. This rebuilds with
# 512-byte chunks so those states occur every few nodes, then runs an
# rc-shaped corpus under ASan. NOTE: rebuilds twice (stress flags in, then
# fclean — the object tree is not keyed on EXTRA_CFLAGS).
arena-stress:  ## Arena chunk-boundary stress under ASan (issue #94 detector)
	@sh $(TEST_DIR)/alloc_stress.sh

# The update path end to end against a LOCAL fake release server: discovery,
# download, sha256, atomic replace, and every rejection path (bad checksum,
# truncated asset, unreachable source). Nothing is installed system-wide and
# no network is touched. update_ui_test drives a pty for the two interactive
# requirements: the header must be lazy, and a discovered update must never
# disturb a line the user is typing.
##@ Feature gates
update-test: all  ## The whole update path against a LOCAL fake release server
	@python3 $(TEST_DIR)/update_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)
	@python3 $(TEST_DIR)/update_ui_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# The prompt prefix must reach the tty in ONE write (real pty). Streaming it
# byte by byte let the line discipline echo type-ahead INTO a colour escape;
# a letter is a valid CSI final byte, so the sequence ended early and its
# tail printed as text (`38;2;112`). See tests/prompt_atomic_test.py.
prompt-atomic-test: all  ## pty: the prompt prefix reaches the tty in ONE write
	@chmod +x $(TEST_DIR)/prompt_atomic_test.py
	@python3 $(TEST_DIR)/prompt_atomic_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# The prompt against a NON-BLOCKING terminal (issue #34). O_NONBLOCK lives on
# the open file description the shell shares with every program it launches,
# so one tool that sets it and never restores it leaves the shell writing to
# a non-blocking tty. tty_write_all used to treat the resulting EAGAIN as a
# hard error and drop the WHOLE frame. The last two checks fill the output
# queue for real before the shell draws, so they fail outright if the EAGAIN
# wait is removed. See tests/nonblock_tty_test.py.
nonblock-tty-test: all  ## pty: prompt frames survive a non-blocking terminal
	@chmod +x $(TEST_DIR)/nonblock_tty_test.py
	@python3 $(TEST_DIR)/nonblock_tty_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# /dev/tcp and /dev/udp redirections vs bash --posix. Brings up its own TCP
# and UDP peer, so it needs python3 but no network access.
net-redir-test: all  ## /dev/tcp and /dev/udp redirections (brings up its own peer)
	@chmod +x $(TEST_DIR)/net_redir_test.py
	@python3 $(TEST_DIR)/net_redir_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Command-line option parsing (-e, -o name, +c, flags after -c, --/-,
# invalid-option status, $-, mode-dependent nounset) vs bash --posix. These
# exercise how the shell parses its own argv, which the golden -c harness
# cannot reach. Host-side, no docker. See tests/cli_opts_compare.sh.
cli-opts-test: all  ## The shell's own argv parsing (-e, -o name, +c, --, $-)
	@chmod +x $(TEST_DIR)/cli_opts_compare.sh
	@HELLISH=$(BIN_DIR)/$(BAPTIZE_SHELL) bash $(TEST_DIR)/cli_opts_compare.sh

# Login-shell startup files: a login hellish must source /etc/profile (which
# runs the /etc/profile.d snippets) and then ~/.profile, exactly as bash does,
# and a non-login one must source neither. The golden -c harness only ever
# spawns non-login shells, so it cannot see this. See tests/login_profile_compare.sh.
login-test: all  ## Login shells source /etc/profile then ~/.profile; others neither
	@chmod +x $(TEST_DIR)/login_profile_compare.sh
	@HELLISH=$(BIN_DIR)/$(BAPTIZE_SHELL) bash $(TEST_DIR)/login_profile_compare.sh

# The no-sudo install route, end to end against a TEMPORARY $$HOME. It put the
# binary in $$PREFIX/bin and exec'd it by absolute path from your login rc --
# so the shell came up, and nobody noticed that neither route ever put that
# directory on PATH. `hellish update`, `command -v hellish`, anything that
# looks the shell up by NAME: command not found, on a machine that had just
# installed it. Covers the PATH block in ~/.hellishrc and in the rc hook, the
# re-source guard, a custom PREFIX, an existing ~/.hellishrc being left alone,
# re-install idempotence and uninstall. CI runs it through `make pty-test`
# (tests/*.py is discovered) as well as by name in the gates job.
user-install-test: all  ## user-install leaves `hellish` on PATH, not just on disk
	@python3 $(TEST_DIR)/user_install_path_test.py $(BIN_DIR)/$(BAPTIZE_SHELL)

# Third-party conformance sweep: Oils spec tests + mksh check.t, run against
# hellish, bash --posix and dash; report in bench/conformance.md; the gate
# fails if hellish's pass count drops vs bench/baseline/. Suites are fetched
# once by bench/conformance.sh's helpers (see bench/README.md).
conformance:  ## Third-party suites (Oils spec + mksh check.t) vs bash AND dash
	@/bin/bash bench/conformance.sh

# Dimension-split speed benchmark (startup / parse / loops / forks /
# configure) vs bash --posix and dash, via pinned hyperfine runs, followed by
# the peak-RSS dimension over the same workloads.
# Reports land in bench/results.md; methodology in bench/METHODOLOGY.md.
perf:  ## Dimension-split hyperfine bench → bench/results.md
	@/bin/bash bench/run.sh
	@/bin/bash bench/lib/run_rss.sh

# Peak-RSS dimension on its own (run.sh must have built bench/.bin/hellish).
rss:  ## Peak-RSS dimension alone (needs a prior make perf)
	@/bin/bash bench/lib/run_rss.sh

# Turn whatever harness output is on disk into bench/charts/*.svg (the images
# the README embeds). Reads every artifact it can find and skips the rest, so
# it is safe to run after a single harness; run `make perf conformance bench`
# first for a full set. Never re-runs a benchmark itself -- charting and
# measuring stay separate so a chart can always be regenerated for free.
charts:  ## Regenerate bench/charts/*.svg from harness output on disk
	@python3 bench/lib/collect_data.py
	@python3 bench/lib/gen_charts.py
	@python3 bench/lib/gen_matrix_chart.py

# Run an external, configurable 42 "minishell tester" (geoman-style) against
# the built binary, as an independent cross-check on top of `make test` and
# `make conformance`. Override the repo with `make geoman GEOMAN_URL=...`.
geoman: all  ## External 42 minishell tester, as an independent cross-check
	@/bin/bash bench/lib/run_geoman.sh

.PHONY: test bench re all clean fclean norm my_shell help help-targets safe_banner flags \
	static static-verify \
	docker-build docker-test docker-alpine docker-debian docker-ubuntu \
	docker-arch docker-fedora docker-rocky docker-opensuse docker-void \
	smoke docker-clean cd-zsh-test cd-posix-test my-shell-test doctor \
	docs-builtins \
	my-shell-uninstall my-shell-purge ssh-shell-test installer-test \
	agnostic-bench \
	hist-test history-opts-test history-matrix-test pty-test git-star-test \
	completion-test completion-posix-test \
	readline-test anim-test git-prompt-test \
	prompt-atomic-test \
	bg-tty-test prompt-integrity-test update-badge-test nonblock-tty-test \
	update-config-test update-test help-test test-release arena-stress \
	conformance perf rss \
	charts cli-opts-test net-redir-test login-test user-install-test \
	geoman oracle docker-suite docker widechar-test \
	user-install user-uninstall
