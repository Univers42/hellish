/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_ps1c.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* The hellish PS1 extension escapes, all SELF-SPACING conditionals: they
   render nothing at all until they have something to say, so a theme can
   chain them and never show empty scaffolding. \g and \S live in
   prompt_ps1b.c; here are the dispatcher and the two timing badges. */

/* \p: " took N.Ns" once the last foreground command crossed 2 seconds —
   long enough to skip instant-command noise, short enough to catch
   anything you actually waited on (same rule as the built-in theme). */
void	ps1_duration(t_shell *state, t_string *out)
{
	char		buf[32];
	long long	s;

	if (state->last_cmd_ms < 2000)
		return ;
	s = state->last_cmd_ms / 1000;
	if (s < 60)
		snprintf(buf, sizeof(buf), " took %lld.%llds", s,
			(state->last_cmd_ms % 1000) / 100);
	else
		snprintf(buf, sizeof(buf), " took %lldm%02llds", s / 60, s % 60);
	vec_push_str(out, buf);
}

/* \J: " ⚙N" while background jobs exist, nothing otherwise (unlike the
   bash-compatible \j, which always prints a number, zero included). */
void	ps1_jobs(t_shell *state, t_string *out)
{
	char	*n;

	if (state->job_table.count <= 0)
		return ;
	vec_push_str(out, " \xe2\x9a\x99");
	n = ft_itoa(state->job_table.count);
	if (n)
		vec_push_str(out, n);
	xfree(n);
}

/* \B: the whole built-in two-row prompt, as one escape.

   This exists so the default prompt can BE a PS1 value instead of the
   absence of one. Python's venv activate does

       _OLD_VIRTUAL_PS1="${PS1:-}" ; PS1="(venv) ${PS1:-}"

   and deactivate restores it only `if [ -n "$_OLD_VIRTUAL_PS1" ]`. With
   PS1 unset that saved value is the empty string, the guard is false, and
   the prompt stays "(venv) " for the rest of the session -- issue #39.
   Every shell that ships a default PS1 round-trips this correctly, so now
   we ship one too: "\B", which renders exactly what an unset PS1 used to. */
void	ps1_builtin(t_shell *state, t_string *out)
{
	render_prompt(state, out, *anim_frame(),
		state->last_cmd_st_exe.status);
}

/* Extension dispatch, tried after the bash-compatible set: \g branch,
   \S failure badge, \p duration, \J jobs, \A animation frame, \U pending
   update, \B the built-in prompt. Returns
   false so unknown escapes still fall through to literal passthrough. */
bool	ps1_escape_ext(t_shell *state, t_string *out, char c)
{
	if (c == 'B')
		return (ps1_builtin(state, out), true);
	if (c == 'g')
		return (ps1_git(out), true);
	if (c == 'S')
		return (ps1_status(state, out), true);
	if (c == 'p')
		return (ps1_duration(state, out), true);
	if (c == 'J')
		return (ps1_jobs(state, out), true);
	if (c == 'A')
		return (ps1_anim(state, out), true);
	if (c == 'U')
		return (ps1_update(state, out), true);
	return (false);
}
