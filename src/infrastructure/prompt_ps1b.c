/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_ps1b.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"

/* \h (short) and \H (full) hostname for PS1. Cached: the box does not
   change its name mid-session, and the prompt renders per command. */
void	ps1_host(t_string *out, char kind)
{
	static char	full[256];
	char		shortname[256];
	char		*dot;

	if (!full[0])
	{
		if (gethostname(full, sizeof(full) - 1) != 0)
			ft_strlcpy(full, "host", sizeof(full));
		full[sizeof(full) - 1] = '\0';
	}
	if (kind == 'H')
		return ((void)vec_push_str(out, full));
	ft_strlcpy(shortname, full, sizeof(shortname));
	dot = ft_strchr(shortname, '.');
	if (dot)
		*dot = '\0';
	vec_push_str(out, shortname);
}

/* \u: the username, from the live environment. */
void	ps1_user(t_shell *state, t_string *out)
{
	char	*user;

	user = env_expand(state, "USER");
	if (!user)
		user = "user";
	vec_push_str(out, user);
}

/* \g (hellish extension): the git branch of the cwd's repo with a dirty
   star, or nothing outside a repo. Rides the built-in prompt's fork-free
   cached reader, so an rc-file theme gets the same zero-cost git segment
   the default theme has. */
void	ps1_git(t_string *out)
{
	char	*branch;
	int		dirty;

	get_git_info(&branch, &dirty);
	if (!branch)
		return ;
	vec_push_str(out, branch);
	if (dirty)
		vec_push_str(out, "*");
	xfree(branch);
}

/* \S (hellish extension): " ✘N" when the last command failed with N,
   nothing at all after success — the leading space is built in so the
   badge spaces itself only when visible, and wrapping the escape in a
   red \[\e[…m\] span costs no width when the badge is empty. */
void	ps1_status(t_shell *state, t_string *out)
{
	char	*n;

	if (state->last_cmd_st_exe.status == 0)
		return ;
	vec_push_str(out, " \xe2\x9c\x98");
	n = ft_itoa(state->last_cmd_st_exe.status);
	if (n)
		vec_push_str(out, n);
	xfree(n);
}

/* $NAME / ${...} inside PS1: expanded from the live environment at every
   render, so `PS1='${VIRTUAL_ENV} \w> '` tracks changes without re-sourcing
   the rc file. Special single-char parameters ($?, $$, ...) are left to
   the shell proper — a prompt string wants variables, and bash's promptvars
   behaviour for plain names is what users expect. Invalid syntax keeps the
   dollar literal.

   A braced form is handed to ps1_brace (prompt_ps1d.c) rather than read
   here: this reader only ever understood a bare name, so an operator like
   ${v:+w} left its own tail on the screen (issue #41).

   Single-character SPECIAL parameters are read here too. They used to fall
   through to literal text, because the guard below is is_var_name_p1() --
   isalpha or '_' -- which '?' is not. So `PS1='[$?] '` rendered the two
   characters `$?` on screen, forever, while `PS1='[${?}] '` was correct:
   the braced path already routed into the real expander. Issue #69 hit the
   other half of it -- a DOUBLE-quoted PS1 froze $? at assignment time, so
   the prompt showed a permanent 0 and looked like the shell was wrong about
   the exit status. One character of dispatch fixes both spellings; the
   value still comes from env_expand_n, so there is one expander, not two. */
void	ps1_dollar(t_shell *state, t_string *out, const char *f, int *i)
{
	int		j;
	char	*val;

	if (f[*i + 1] == '{')
		return (ps1_brace(state, out, f, i));
	j = *i + 1;
	if (ps1_is_special(f[j]))
	{
		val = env_expand_n(state, (char *)f + j, 1);
		if (val)
			vec_push_str(out, val);
		return ((void)(*i = j + 1));
	}
	if (!is_var_name_p1(f[j]))
		return (vec_push_char(out, '$'), (void)(*i += 1));
	*i = j;
	while (is_var_name_p2(f[j]))
		j++;
	val = env_expand_n(state, (char *)f + *i, j - *i);
	if (val)
		vec_push_str(out, val);
	*i = j;
}
