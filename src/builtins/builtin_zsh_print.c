/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_print.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 20:15:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 20:15:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>

t_string	zsh_prompt(t_shell *state, char *fmt, bool strict);

/* print [-nrlP] [--] [arg ...] -- zsh's echo, and a separate builtin rather
** than an alias for ours because the DEFAULT differs: `echo` here leaves
** escapes alone unless -e says otherwise, `print` always processes them
** unless -r says otherwise.  Aliasing one to the other would quietly turn
** every `print "a\tb"` in a plugin into a literal backslash and a t.
**
**   -r -R  raw: no escape processing      -n   no trailing newline
**   -l     one argument per line          -P   expand prompt escapes
**   --     end of options
**
** Deliberately unsupported, and SAID so: -z (push to the editor buffer),
** -s (push to history), -u and -p (a target fd or coprocess).  Each needs
** machinery that does not exist here, and each would otherwise look like it
** had worked while writing nothing anywhere.
**
** Escapes go through e_parser -- the same decoder `echo -e` uses, with the
** one flag on which the two shells disagree (an unknown escape keeps its
** backslash for echo and loses it for print) -- and the
** whole line is built in one buffer for a single write(2), both for the same
** reason: one behaviour, one syscall, no second implementation to drift.
*/
/* One option letter. Non-zero means "stop, already reported". */
static int	print_one_flag(t_shell *state, char c, t_pflags *f)
{
	if (c == 'r' || c == 'R')
		return (f->raw = true, 0);
	if (c == 'n')
		return (f->nonl = true, 0);
	if (c == 'l')
		return (f->lines = true, 0);
	if (c == 'P')
		return (f->prompt = true, 0);
	return (ft_eprintf("%s: print: -%c: not supported\n", state->ctx, c), 1);
}

/* Consume the option words.  Returns the index of the first operand, or -1
   after reporting an option we do not implement. */
static int	print_flags(t_shell *state, t_vec argv, t_pflags *f)
{
	char	*w;
	size_t	i;
	int		j;

	i = 1;
	while (i < argv.len)
	{
		w = ((char **)argv.ctx)[i];
		if (w[0] != '-' || !w[1])
			break ;
		if (!ft_strcmp(w, "--"))
			return ((int)i + 1);
		j = 0;
		while (w[++j])
			if (print_one_flag(state, w[j], f))
				return (-1);
		i++;
	}
	return ((int)i);
}

/* One operand into the buffer.  -P first, then -r: prompt escapes are part
   of the text, and -r governs the BACKSLASH escapes, so `%F{red}` still
   renders under -r while `\t` does not.

   The \001/\002 bytes are readline's zero-width markers, meaningful only
   around a live prompt; zsh's print -P emits the escape sequences bare
   (measured on the 5.9 oracle), so they are stripped here -- and the
   parity test diffs the two outputs byte for byte.
   HELLISH_DBG_PROMPT_MARKS keeps them: the width tests exist to see
   exactly those bytes, and this is their only window. */
static void	print_one(t_shell *state, t_pflags *f, t_string *out, char *s)
{
	t_string	r;
	char		*p;

	if (f->prompt)
	{
		r = zsh_prompt(state, s, true);
		p = (char *)r.ctx;
		while (p && *p)
		{
			if ((*p != '\001' && *p != '\002')
				|| getenv("HELLISH_DBG_PROMPT_MARKS"))
				vec_push_char(out, *p);
			p++;
		}
		if (r.ctx)
			return ((void)xfree(r.ctx));
	}
	if (f->raw)
		vec_push_str(out, s);
	else
		e_parser(out, s, true);
}

/* Gather every operand into one buffer: -l separates with newlines, the
   default with spaces, and -n decides whether the last one gets a newline
   of its own. */
static void	print_gather(t_shell *state, t_pflags *f, t_vec argv, t_string *out)
{
	int	first;
	int	i;

	first = print_flags(state, argv, f);
	i = first - 1;
	while (++i < (int)argv.len)
	{
		if (i > first && f->lines)
			vec_push_char(out, '\n');
		else if (i > first)
			vec_push_char(out, ' ');
		print_one(state, f, out, ((char **)argv.ctx)[i]);
	}
	if (!f->nonl)
		vec_push_char(out, '\n');
}

int	builtin_print(t_shell *state, t_vec argv)
{
	t_pflags	f;
	t_string	out;

	f = (t_pflags){0};
	if (print_flags(state, argv, &f) < 0)
		return (1);
	vec_init(&out);
	out.elem_size = 1;
	print_gather(state, &f, argv, &out);
	if (out.len > 0 && write(STDOUT_FILENO, out.ctx, out.len) < 0)
		return (xfree(out.ctx), ft_eprintf("%s: print: write error\n",
				state->ctx), 1);
	return (xfree(out.ctx), 0);
}
