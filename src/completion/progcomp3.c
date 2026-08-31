/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   progcomp3.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 10:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 10:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "progcomp_private.h"
#include "helpers.h"
#include <readline/readline.h>

/* The variables bash promises a completion function, spelled as shell text.

   COMP_WORDS is where the divergence would hide if this were done by hand:
   scripts read ${COMP_WORDS[COMP_CWORD-1]} far more often than they read
   $3, so an index that is off by one produces completions for the WRONG
   flag -- plausible output, never an error. */

/* Append one value, single-quoted so the parser reads it back verbatim.
   A NULL is an empty string rather than a crash: every caller here is
   handing over user-controlled text from a half-typed line. */
void	pc_qpush(t_string *out, const char *s, int n)
{
	char	*raw;
	char	*q;

	if (!s)
	{
		vec_push_str(out, "''");
		return ;
	}
	raw = ft_strndup((char *)s, (size_t)n);
	q = sq_quote(raw);
	vec_push_str(out, q);
	xfree(raw);
	xfree(q);
}

/* Emit every whitespace-separated word of the line, quoted and space
   separated; returns how many, and sets *cw to how many began before the
   cursor -- which is the index of the word being completed. */
static int	pc_emit_words(t_string *out, int start, int *cw)
{
	int	i;
	int	b;
	int	n;

	i = 0;
	n = 0;
	*cw = 0;
	while (rl_line_buffer[i])
	{
		while (rl_line_buffer[i] == ' ' || rl_line_buffer[i] == '\t')
			i++;
		b = i;
		while (rl_line_buffer[i] && rl_line_buffer[i] != ' '
			&& rl_line_buffer[i] != '\t')
			i++;
		if (i == b)
			break ;
		*cw += (b < start);
		pc_qpush(out, rl_line_buffer + b, i - b);
		vec_push_char(out, ' ');
		n++;
	}
	return (n);
}

/* COMP_LINE, COMP_POINT, COMP_WORDS, COMP_CWORD, and an emptied COMPREPLY.
   The empty trailing element matters: with the cursor after a space the
   word being completed does not exist yet, and bash still puts a "" in
   COMP_WORDS for it so that COMP_CWORD indexes something. */
void	pc_head(t_string *out, int start)
{
	char	*n;
	int		cw;
	int		cnt;

	vec_push_str(out, "COMP_LINE=");
	pc_qpush(out, rl_line_buffer, (int)ft_strlen(rl_line_buffer));
	n = ft_itoa(rl_point);
	vec_push_str(out, "; COMP_POINT=");
	vec_push_str(out, n);
	xfree(n);
	vec_push_str(out, "; COMP_WORDS=(");
	cnt = pc_emit_words(out, start, &cw);
	if (cnt == cw)
		vec_push_str(out, "'' ");
	n = ft_itoa(cw);
	vec_push_str(out, "); COMP_CWORD=");
	vec_push_str(out, n);
	xfree(n);
	vec_push_str(out, "; COMPREPLY=(); ");
}
