# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    variables.mk                                       :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/10/23 19:04:34 by dlesieur          #+#    #+#              #
#    Updated: 2025/11/04 20:39:43 by dlesieur         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CC				:=	cc
NAME			:=	shell
DPDCS			:=	libft.a
# Language standard and feature macros
STD				:=	-std=gnu99
DEFINES			:=	-D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L

# Warning and hardening flags (keep strictness)
WARN_FLAGS		:=	-Wall -Wextra -Werror -pedantic \
					-Wshadow -Wpointer-arith -Wcast-align -Wvolatile-register-var \
					-Wdangling-else -Wenum-compare -Wexpansion-to-defined -Waddress \
					-Wno-shift-count-overflow -Wstrict-overflow -Wfatal-errors

# Analyzer / sanitizers (kept as a dedicated variable)
ANAFLAGS		:=	-fsanitize=address,undefined,leak

# Preprocessor flags (kept empty — dependency flags are emitted per-compile)
PREPROCFLAGS	:=	-MMD -MP -MF
CPPFLAGS		:=	$(DEFINES)

# Debug flags (overridable) and optimization flags
DEBFLAGS		?=	-g3 -O0 -ggdb
OPTFLAGS		:=	-Ofast -pipe

# Final CFLAGS composition (keeps strict warnings and optional analyzers)
CFLAGS			:=	$(STD) $(WARN_FLAGS) $(DEBFLAGS) $(CPPFLAGS) $(ANAFLAGS)

# Linker flags
LDFLAGS			:=	-lreadline -pthread -lm $(ANAFLAGS)
