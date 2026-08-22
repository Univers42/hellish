# **************************************************************************** #
#                                                                              #
#    mk/colors.mk — the palette libft's mk/functions.mk expects                 #
#                                                                              #
# **************************************************************************** #

# Why this file exists instead of `include vendor/libft/mk/global_conf.mk`.
#
# The wrappers in libft's mk/functions.mk (log_ok, log_info, log_title, …) are
# worth reusing, and this Makefile does include that file. Its colour palette
# lives in global_conf.mk — and so do these:
#
#     CC       := cc
#     OBJ_DIR  := obj
#     RM       := rm -rf
#     LDFLAGS  := $(DPDCS_STATIC)
#     CFLAGS   ?= $(CSTD) $(WARN_FLAGS) $(DEBFLAGS) $(ANAFLAGS) $(DEFINES)
#
# Measured, not guessed: a Makefile whose only content is that include reports
# `CC=cc OBJ_DIR=obj RM=rm -rf`. Every one of those is a variable this build
# depends on, so including the file would couple hellish's *build* to a
# sibling repo's defaults — an edit over there would silently change what gets
# compiled here, and `RM := rm -rf` in particular turns every `$(RM)` in this
# tree into a recursive delete.
#
# libft's mk/common.mk is worse for the purpose: it defines real targets
# (`deps`, `ensure_lib_deps`, `build_tests`), and a target defined by an
# include can become the default goal depending on include order.
#
# So: the wrappers come from libft, the palette is declared here, and the build
# variables stay ours. The names below are exactly the ones functions.mk
# references — if it grows a new colour, add it here.

ESC          := $(shell printf '\033')

RESET        := $(ESC)[0m
BOLD         := $(ESC)[1m
WHITE        := $(ESC)[0;37m

BOLD_GREEN   := $(ESC)[1;32m
BOLD_YELLOW  := $(ESC)[1;33m
BOLD_MAGENTA := $(ESC)[1;35m
BOLD_CYAN    := $(ESC)[1;36m
BOLD_WHITE   := $(ESC)[1;37m

BRIGHT_RED   := $(ESC)[1;31m
BRIGHT_GREEN := $(ESC)[1;32m
BRIGHT_YELLOW:= $(ESC)[1;33m
BRIGHT_CYAN  := $(ESC)[1;36m

DIM          := $(ESC)[2m
GRAY         := $(ESC)[0;90m

# functions.mk interpolates this one into every print_status/logging call. It
# is commented out in libft's own global_conf.mk, so it expands to nothing
# there; giving it a real value is the difference between a readable prefix and
# a bare one.
FADED_BOLD_GRAY := $(ESC)[1;2;37m

# Honour NO_COLOR (https://no-color.org) and a non-tty stdout: a build log
# scraped by CI should not be full of escape sequences, and grep should be able
# to match what it sees.
ifdef NO_COLOR
COLOR_OFF := 1
endif
ifeq ($(shell test -t 1 || echo notty),notty)
COLOR_OFF := 1
endif
ifdef COLOR_OFF
ESC :=
RESET :=
BOLD :=
WHITE :=
BOLD_GREEN :=
BOLD_YELLOW :=
BOLD_MAGENTA :=
BOLD_CYAN :=
BOLD_WHITE :=
BRIGHT_RED :=
BRIGHT_GREEN :=
BRIGHT_YELLOW :=
BRIGHT_CYAN :=
DIM :=
GRAY :=
FADED_BOLD_GRAY :=
endif
