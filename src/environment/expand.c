/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:02:38 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:02:38 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Variable expansion entry points: both special shell variables ($?, $$,
   $!, $-, $#, $LINENO, positional params $1..$N) and ordinary env vars.
   env_expand_n is the hot path -- called thousands of times per script.
   It intentionally returns interior pointers into state or the env vector,
   NOT copies, so callers must NOT free the result. */

#include "env.h"
#include "shell.h"
#include "helpers.h"
#include "sh_input.h"

bool	is_readonly_var(t_shell *state, const char *key);
void	exit_clean(t_shell *state, int code);
char	*build_flagstr(t_shell *state);
char	*lineno_str(t_shell *state);
char	*expand_special_dyn(t_shell *state, char *key, int len);

/* Detect a positional parameter index: key must be all digits and > 0.
   $0 is NOT a positional parameter -- POSIX says $0 is the shell name,
   stored as a plain env var so it falls through to the normal lookup. */
static int	pos_index(const char *key, int len)
{
	int	n;
	int	i;

	if (len <= 0)
		return (-1);
	n = 0;
	i = 0;
	while (i < len)
	{
		if (key[i] < '0' || key[i] > '9')
			return (-1);
		n = n * 10 + (key[i] - '0');
		i++;
	}
	if (n < 1)
		return (-1);
	return (n);
}

/* Map the one-character special variables to their runtime values.
   Returns NULL (not "not found") to distinguish "keep looking in env"
   from "found, value is empty string" -- both "" and NULL are valid. */
static char	*expand_special(t_shell *state, char *key, int len)
{
	if (ft_strncmp(key, "?", len) == 0 && len == 1)
		return (state->last_cmd_st);
	if (ft_strncmp(key, "$", len) == 0 && state->pid && len == 1)
		return (state->pid);
	if (key[0] == '!' && len == 1)
	{
		if (state->last_bg_pid)
			return (state->last_bg_pid);
		return ("");
	}
	if (len == 1 && key[0] == '-')
		return (build_flagstr(state));
	if (len == 0)
		return ("");
	if (len == 1 && key[0] == '#')
	{
		if (state->pos.cnt_str[0])
			return (state->pos.cnt_str);
		return ("0");
	}
	return (expand_special_dyn(state, key, len));
}

/* Expand $key (len bytes): specials first, then positionals, then env.
   Returns a borrowed pointer -- callers must not free it.  Returns NULL
   if unset (as opposed to "" for set-but-empty; distinction matters for
   ${v:?} and opt_nounset). */
char	*env_expand_n(t_shell *state, char *key, int len)
{
	t_env	*curr;
	char	*sp;
	int		n;

	sp = expand_special(state, key, len);
	if (sp != NULL)
		return (sp);
	n = pos_index(key, len);
	if (n >= 1)
	{
		if (n <= state->pos.count)
			return (state->pos.args[n - 1]);
		return (0);
	}
	curr = env_nget(&state->env, key, len);
	if (curr == 0 || curr->key == 0)
		return (0);
	return (curr->value);
}

char	*env_expand(t_shell *state, char *key)
{
	return (env_expand_n(state, key, ft_strlen(key)));
}

/* Drain `src` into `dest`, overriding each entry's exported flag.
   Used when a subshell or function merges its local assignments back.
   src is emptied and freed; dest owns everything afterwards. */
void	env_extend(t_vec_env *dest, t_vec_env *src, bool export)
{
	t_env	curr;

	while (src->len)
	{
		curr = *(t_env *)vec_pop(src);
		if (!curr.key)
			continue ;
		curr.exported = export;
		env_set(dest, curr);
	}
	xfree(src->ctx);
	vec_init(src);
}
