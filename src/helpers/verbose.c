/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   verbose.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 18:12:50 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 18:35:12 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"

/* VERBOSE build: forward to claptrap() which knows which subsystem flags
   (lexer, parser, AST...) to filter on. The flag argument matches the
   OPT_FLAG_DEBUG_* bits so callers can gate at the right verbosity level. */
#ifdef VERBOSE

void	verbose(int flag, const char *str, ...)
{
	va_list	args;

	va_start(args, str);
	claptrap(flag, str, args);
	va_end(args);
}

/* Non-VERBOSE build: swallow all arguments and disappear. The compiler sees
   (void)flag; (void)str; so -Wunused-parameter stays silent, and the va_list
   is never touched -- no overhead at all in release builds. */
#else

void	verbose(int flag, const char *str, ...)
{
	(void)flag;
	(void)str;
}

#endif
