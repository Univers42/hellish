/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias_scan.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"
#include "shell.h"

/* Push an alias value as a new scan source.  The alias name is recorded so
   asc_active can veto re-expansion of a name already being expanded (the
   POSIX loop guard), and blank_end drives the "value ends in a blank =>
   also check the next word" rule when this source is later popped. */
bool	asc_push(t_ascan *a, const char *name, size_t nlen, const char *val)
{
	t_ascan_src	*s;
	size_t		vlen;

	if (a->depth >= ALIAS_SCAN_DEPTH)
		return (false);
	s = &a->src[a->depth];
	s->s = val;
	s->pos = 0;
	s->name = ft_substr(name, 0, nlen);
	vlen = ft_strlen(val);
	s->blank_end = vlen > 0
		&& (val[vlen - 1] == ' ' || val[vlen - 1] == '\t');
	a->depth++;
	return (true);
}

/* Pop an exhausted source.  A value that ended in a blank arms chk_next so
   the word that follows the whole expansion is alias-checked even though
   it is not in command position. */
void	asc_pop(t_ascan *a)
{
	t_ascan_src	*s;

	s = &a->src[a->depth - 1];
	if (a->depth > 1 && s->blank_end)
		a->chk_next = true;
	if (s->name)
		xfree(s->name);
	s->name = NULL;
	a->depth--;
}

/* A newline inside a spliced alias value would break the driver's
   one-statement-list-per-cycle model (everything after the first
   statement would be dropped), so it is rewritten exactly like the
   cmdhist joiner rewrites history: "; " where a separator is valid, a
   plain space where the grammar still expects a command (right after
   do/then/{/;/&&...) — which is precisely what cmd_pos tracks. */
void	asc_emit_sep(t_ascan *a)
{
	if (!a->cmd_pos)
		vec_push_nstr(&a->out, ";", 1);
	vec_push_nstr(&a->out, " ", 1);
}

/* One full scan of a logical input line: aliases in command position are
   textually replaced before the tokenizer ever runs, which is the only
   stage where quoting, keywords in alias bodies, and recursive values can
   all behave like bash.  Returns an xmalloc'd NUL-terminated string. */
char	*alias_scan_line(t_hash *aliases, const char *input)
{
	t_ascan		a;
	t_ascan_src	*s;
	char		c;

	if (!input)
		return (NULL);
	if (alias_table_empty(aliases))
		return (ft_strdup(input));
	a = (t_ascan){0};
	vec_init(&a.out);
	a.out.elem_size = 1;
	a.aliases = aliases;
	a.cmd_pos = true;
	a.wstart = true;
	a.src[0] = (t_ascan_src){.s = input};
	a.depth = 1;
	while (a.depth > 0)
	{
		s = &a.src[a.depth - 1];
		c = s->s[s->pos];
		if (c == '\0')
			asc_pop(&a);
		else if (c == ' ' || c == '\t')
			asc_scan_blank(&a, s, c);
		else if (c == '\n')
			asc_newline(&a);
		else if (c == '#' && a.wstart)
			asc_comment(&a);
		else if (ft_strchr(";|&()<>", c))
			asc_op(&a);
		else
			asc_word(&a);
	}
	c = '\0';
	vec_push(&a.out, &c);
	while (a.hd_n > 0)
		asc_hd_drop(&a);
	return ((char *)a.out.ctx);
}

/* Refresh state->alias_exp: the tokenizer parses this expanded copy while
   state->input keeps the user's original bytes for history and errors.
   With no aliases defined the expansion is a no-op, so instead of strdup'ing
   the whole file we BORROW input.ctx (alias_exp_owned=false) -- the lexer
   never mutates the buffer (tokens are slices) so input stays pristine, and
   we save a full-file copy (~1.7MB on a 50k-line script). The previous
   alias_exp is freed only when we owned it; a borrow is left for input's own
   teardown. input.ctx moves under vec regrowth between reads, but this runs
   after every read and re-borrows the current pointer. */
void	alias_scan_update(t_shell *state)
{
	if (state->alias_exp_owned && state->alias_exp.ctx)
		xfree(state->alias_exp.ctx);
	state->alias_exp = (t_string){0};
	state->alias_exp_owned = false;
	if (!state->input.ctx)
		return ;
	state->alias_exp.elem_size = 1;
	if (alias_table_empty(&state->aliases))
	{
		state->alias_exp.ctx = state->input.ctx;
		state->alias_exp.len = ft_strlen((char *)state->input.ctx);
		return ;
	}
	state->alias_exp.ctx
		= alias_scan_line(&state->aliases, (char *)state->input.ctx);
	state->alias_exp.len = ft_strlen((char *)state->alias_exp.ctx);
	state->alias_exp_owned = true;
}
