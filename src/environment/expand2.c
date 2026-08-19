/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:02:38 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:02:38 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Continuation of expand.c: readonly guard for assignments, and the
   env_apply_cmd_assigns() batch used by the executor when applying
   NAME=VALUE pre-command assignments. */

#include "env.h"
#include "shell.h"
#include "helpers.h"
#include "sh_input.h"
#include "ft_stdlib.h"
#include <time.h>

bool	is_readonly_var(t_shell *state, const char *key);
void	exit_clean(t_shell *state, int code);
int		shell_fatal_status(t_shell *state);
char	*lineno_str(t_shell *state);

int		tok_lineno(t_shell *state);

/* Dynamic special variables computed at expansion time, second tier of
   expand_special: $LINENO, plus the bash-isms $RANDOM (0..32767 from the
   session PRNG), $SECONDS (whole seconds since shell start) and
   $EPOCHSECONDS. All borrow state->linebuf like lineno_str. A user
   assignment to these names does NOT shadow them (bash lets SECONDS=N
   re-base the counter; accepted divergence). NULL = not one of ours. */
char	*expand_special_dyn(t_shell *state, char *key, int len)
{
	if (len == 6 && ft_strncmp(key, "LINENO", 6) == 0)
		return (lineno_str(state));
	if (len == 6 && ft_strncmp(key, "RANDOM", 6) == 0)
	{
		snprintf(state->linebuf, sizeof(state->linebuf), "%u",
			random_uint32(&state->prng) & 32767u);
		return (state->linebuf);
	}
	if (len == 7 && ft_strncmp(key, "SECONDS", 7) == 0)
	{
		snprintf(state->linebuf, sizeof(state->linebuf), "%lld",
			(long long)time(NULL) - state->start_sec);
		return (state->linebuf);
	}
	if (len == 12 && ft_strncmp(key, "EPOCHSECONDS", 12) == 0)
	{
		snprintf(state->linebuf, sizeof(state->linebuf), "%lld",
			(long long)time(NULL));
		return (state->linebuf);
	}
	return (NULL);
}

/* $LINENO: the line number currently being read/executed (input-line
   granularity, like bash for multiple commands on one source line).
   ft_itoa allocates; we copy into a small static buffer so we can return
   a borrowed pointer with no lifetime headache, then free the temp. */
char	*lineno_str(t_shell *state)
{
	char	*s;

	s = ft_itoa(tok_lineno(state));
	if (!s)
		return (ft_strlcpy(state->linebuf, "0", 2), state->linebuf);
	ft_strlcpy(state->linebuf, s, sizeof(state->linebuf));
	xfree(s);
	return (state->linebuf);
}

/* Emit the POSIX-mandated error, then discard both key and value so the
   caller can't accidentally apply a partial assignment.  Outside an
   interactive shell a readonly violation is fatal; the status is 127 only
   for the top-level shell of a -c string, 1 for a script or a subshell
   (shell_fatal_status), which is what bash reports. */
static void	handle_readonly_assign(t_shell *state, t_env *el)
{
	ft_eprintf("%s: %s: readonly variable\n", state->ctx, el->key);
	xfree(el->key);
	xfree(el->value);
	el->key = NULL;
	el->value = NULL;
	if (state->metinp != INP_RL)
		exit_clean(state, shell_fatal_status(state));
	state->last_cmd_st_exe = (t_execution_state){.status = 1};
}

static void	apply_one_assign(t_shell *state, t_env *el, bool export)
{
	if (is_readonly_var(state, el->key))
	{
		handle_readonly_assign(state, el);
		return ;
	}
	el->exported = export;
	env_set(&state->env, *el);
	el->key = NULL;
	el->value = NULL;
}

/* Apply all NAME=VALUE words that prefixed a simple command.  If export
   is true (e.g. the command was `export`), the variables become exported.
   Each applied entry's key is set to NULL after the move so the caller's
   free path knows ownership was transferred. */
void	env_apply_cmd_assigns(t_shell *state,
			t_executable_cmd *src, bool export)
{
	size_t	i;
	t_env	*el;

	if (!state || !src)
		return ;
	i = 0;
	while (i < src->pre_assigns.len)
	{
		el = &((t_env *)src->pre_assigns.ctx)[i];
		if (el->key)
			apply_one_assign(state, el, export);
		i++;
	}
}
