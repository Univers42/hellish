/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_read.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/*   Created: 2026/03/15 02:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 02:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>

/* Snapshot IFS into an owned copy so later env_set calls inside the read
   loop cannot pull the rug out from under the field-splitting code. Unset
   IFS defaults to <space><tab><newline> per POSIX 2018 §2.6.5. */
char	*dup_ifs(t_shell *state)
{
	char	*v;

	v = env_expand(state, "IFS");
	if (!v)
		return (ft_strdup(" \t\n"));
	return (ft_strdup(v));
}

/* True if c is in the IFS string (and not NUL — the NUL guard means we
   never count the empty string as a delimiter). */
int	is_ifs(char c, const char *ifs)
{
	return (c && ft_strchr(ifs, c) != NULL);
}

/* IFS whitespace characters get special treatment during field splitting:
   they are collapsed (multiple adjacent whitespace IFS chars count as one
   delimiter), unlike non-whitespace IFS chars which each split exactly. */
int	is_ifs_ws(char c, const char *ifs)
{
	return ((c == ' ' || c == '\t' || c == '\n') && is_ifs(c, ifs));
}

/* read_one_line (one logical line, block-buffered on seekable fds) lives
   in builtin_read4.c. */
