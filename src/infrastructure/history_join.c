/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_join.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"
#include "libft.h"

/* Join a multi-line command into the single-line form readline can edit,
   with bash `cmdhist` semantics (verified against bash 5.2 in a pty):
   a newline at a command boundary becomes "; ", or a lone " " when a ";"
   would be a syntax error (after ; & | ( { ) < > or a dangling reserved
   word like `then`/`do`/`in`); a top-level \<newline> continuation is
   deleted outright; newlines inside quotes, `$(...)`, `$((...))` and
   here-doc bodies stay literal so the recalled command still means the
   same thing. The old space-for-everything flattening produced commands
   that broke on recall (`for i in 1 2 3 do ... done`) or hung the shell
   (a here-doc joined onto one line never finds its terminator). */

/* Emit the separator for a top-level newline: a space when the joined tail
   ends where a ";" cannot legally follow, "; " otherwise (bash parity). A
   trailing ")" is ambiguous — after a case pattern (`a)`) a ";" is illegal,
   after a closed subshell (`(echo s)`) it is required — so hj_paren leaves
   the cpat flag telling the two apart, exactly as bash 5.2 behaves. */
static void	hj_emit_delim(t_hjoin *h)
{
	size_t	n;
	char	c;

	n = h->out.len;
	while (n > 0 && (((char *)h->out.ctx)[n - 1] == ' '
		|| ((char *)h->out.ctx)[n - 1] == '\t'))
		n--;
	if (n == 0)
		return (vec_push_char(&h->out, ' '));
	c = ((char *)h->out.ctx)[n - 1];
	if (c == ')' && h->cpat)
		vec_push_char(&h->out, ' ');
	else if (c != ')' && (ft_strchr(";&|({<>", c)
			|| hj_last_word_kw((char *)h->out.ctx, n)))
		vec_push_char(&h->out, ' ');
	else
		vec_push_str(&h->out, "; ");
}

/* Classify one bare paren: it nests inside $(( )) / $( ) when one of those
   is open, otherwise it tracks plain subshell depth; a ")" with nothing
   open at all closes a case pattern, which the delimiter must know about. */
static void	hj_paren(t_hjoin *h, char c)
{
	h->cpat = false;
	if (c == '(' && h->arith == 0 && h->csub == 0)
		h->pdepth++;
	else if (c == '(')
		hj_depth_step(h, 1);
	else if (h->arith > 0 || h->csub > 0)
		hj_depth_step(h, -1);
	else if (h->pdepth > 0)
		h->pdepth--;
	else
		h->cpat = true;
}

/* One newline of the original command: keep it literal inside any quoting
   or substitution construct, open the here-doc body when tags are pending,
   and otherwise emit the boundary delimiter. */
static void	hj_newline(t_hjoin *h)
{
	if (h->sq || h->dq || h->btick || h->csub > 0 || h->arith > 0
		|| h->dpar > 0)
		vec_push_char(&h->out, '\n');
	else if (h->tags.len > 0)
	{
		vec_push_char(&h->out, '\n');
		h->body = true;
	}
	else
		hj_emit_delim(h);
	h->i++;
}

/* Track quote state, substitution depth and paren nesting for one ordinary
   character and copy it through. Single-quote mode swallows everything up
   to the closing quote; $(( counts as depth two so its double `))` unwinds
   it; the final guard keeps cpat alive across blanks only (parens are owned
   by hj_paren, every other non-blank byte clears the case-pattern flag). */
static void	hj_track(t_hjoin *h)
{
	char	c;

	c = h->s[h->i];
	if (h->sq)
		h->sq = (c != '\'');
	else if (c == '\'' && !h->dq)
		h->sq = true;
	else if (c == '"')
		h->dq = !h->dq;
	else if (c == '`' && !h->dq)
		h->btick = !h->btick;
	else if (c == '$' && hj_dollar(h))
		return ;
	else if (c == '}' && h->dpar > 0)
		h->dpar--;
	else if ((c == '(' || c == ')') && !h->dq)
		hj_paren(h, c);
	vec_push_char(&h->out, c);
	h->i++;
	if (c != ' ' && c != '\t' && c != '(' && c != ')')
		h->cpat = false;
}

/* Build the single-line readline form of cmd. Returns a heap string the
   caller must xfree, or NULL when cmd is NULL. */
char	*hist_join_line(const char *cmd)
{
	t_hjoin	h;

	if (!cmd)
		return (NULL);
	hj_init(&h, cmd);
	while (h.s[h.i])
	{
		if (h.body)
			hj_heredoc_body(&h);
		else if (h.s[h.i] == '\\' && !h.sq)
			hj_copy_escaped(&h);
		else if (h.s[h.i] == '\n')
			hj_newline(&h);
		else if (hj_at_heredoc(&h))
			hj_heredoc_tag(&h);
		else
			hj_track(&h);
	}
	return (hj_finish(&h));
}
