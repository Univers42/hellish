/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   progcomp4.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 10:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 10:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "progcomp_private.h"
#include "env.h"

int	exec_string(t_shell *state, char *content);

/* Running the spec and reading the answer back out of COMPREPLY. */

/* -F: the function is called with three arguments -- the command, the word
   being completed, and the word before it -- and fills COMPREPLY. That is
   the contract every completion script in the world is written against. */
static void	pc_func_tail(t_string *out, t_compspec *c, const char *text,
		int start)
{
	char	*w;

	vec_push_str(out, c->func);
	vec_push_char(out, ' ');
	w = pc_cmd_word(start);
	pc_qpush(out, w, (int)ft_strlen(w));
	xfree(w);
	vec_push_char(out, ' ');
	pc_qpush(out, text, (int)ft_strlen(text));
	vec_push_char(out, ' ');
	w = pc_prev_word(start);
	pc_qpush(out, w, (int)ft_strlen(w));
	xfree(w);
}

/* -W and the actions both route to compgen, which already knows how to
** generate each of them and already agrees with bash about the exit status.
** Building a second generator here would be the second copy of a thing that
** has to match exactly.
**
** `set -f` brackets the assignment because $(...) inside an array literal
** is word-split AND globbed: a candidate containing a `*` would otherwise
** be replaced by whatever files it happened to match. Turning it back off
** afterwards is cosmetic -- this runs in the readline child, whose option
** state dies with the line -- but a bracket that only opens invites the
** next reader to wonder.
*/
static void	pc_gen_tail(t_string *out, t_compspec *c, const char *text)
{
	vec_push_str(out, "set -f; COMPREPLY=($(compgen");
	if (c->words)
	{
		vec_push_str(out, " -W ");
		pc_qpush(out, c->words, (int)ft_strlen(c->words));
	}
	if (c->act == 'A')
		vec_push_str(out, " -A function");
	else if (c->act)
	{
		vec_push_str(out, " -");
		vec_push_char(out, c->act);
	}
	vec_push_str(out, " -- ");
	pc_qpush(out, text, (int)ft_strlen(text));
	vec_push_str(out, ")); set +f");
}

/* The whole command for one TAB press, or NULL when the spec carries
   nothing to run -- `complete -o nospace cmd` registers decoration and no
   generator, and must not be mistaken for "offer nothing". */
char	*pc_call_str(t_compspec *c, const char *text, int start)
{
	t_string	out;

	vec_init(&out);
	out.elem_size = 1;
	pc_head(&out, start);
	if (c->func)
		pc_func_tail(&out, c, text, start);
	else if (c->words || c->act)
		pc_gen_tail(&out, c, text);
	else
		return (xfree(out.ctx), NULL);
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}

/* Read COMPREPLY into the match list. A scalar assignment (COMPREPLY=foo,
   which scripts do write) is one match, not a parse error. */
static bool	pc_collect(t_shell *st)
{
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;
	char		*val;

	val = env_expand(st, "COMPREPLY");
	if (!val || !*val)
		return (false);
	if (!arr_is(val))
		return (pc_push(val, (int)ft_strlen(val)), true);
	cur = val + 1;
	idx = 0;
	while (arr_next(&cur, &idx, &v, &vl))
		pc_push(v, vl);
	return (pc_cell()->len > 0);
}

bool	pc_build(t_shell *st, t_compspec *c, const char *text, int start)
{
	char	*cmd;

	pc_reset();
	cmd = pc_call_str(c, text, start);
	if (!cmd)
		return (false);
	exec_string(st, cmd);
	xfree(cmd);
	return (pc_collect(st));
}
