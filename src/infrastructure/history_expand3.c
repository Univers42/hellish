/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_expand3.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "history.h"
#include "history_private.h"
#include "libft.h"
#include <stdlib.h>

/* ^old^new[^] : re-run the last command with old -> new (csh quick sub). */
char	*quick_sub(t_shell *state, const char *input)
{
	const char	*c2;
	const char	*c3;
	char		*old;
	char		*nw;
	char		*res;

	c2 = ft_strchr(input + 1, '^');
	if (!c2 || !hist_last(state))
		return (NULL);
	c3 = ft_strchr(c2 + 1, '^');
	old = ft_strndup(input + 1, c2 - (input + 1));
	if (c3)
		nw = ft_strndup(c2 + 1, c3 - (c2 + 1));
	else
		nw = ft_strndup(c2 + 1, ft_strcspn(c2 + 1, "\n"));
	res = replace_first(hist_last(state), old, nw);
	xfree(old);
	xfree(nw);
	if (!res)
		return (ft_eprintf("%s: :s: substitution failed\n", state->ctx),
			ft_strdup(""));
	if (state->hist.quiet_expand)
		return (res);
	return (ft_printf("%s\n", res), res);
}

/* Process one '!' designator at position i in input, appending the resolved
   entry to result and advancing *pi past the designator. Returns NULL on
   success, or an empty-string error if the event was not found (matches bash's
   "event not found" error which also suppresses execution). */
static char	*expand_bang_loop(t_shell *state, t_string *result,
					const char *input, size_t *pi)
{
	size_t	adv;
	char	*sub;
	size_t	i;

	i = *pi;
	adv = 0;
	sub = resolve_bang(state, input + i + 1, &adv);
	if (!sub)
	{
		ft_eprintf("%s: !%.*s: event not found\n",
			state->ctx, (int)adv, input + i + 1);
		xfree(result->ctx);
		return (ft_strdup(""));
	}
	vec_push_str(result, sub);
	*pi = i + 1 + adv;
	return (NULL);
}

/* True when input[i] starts an expandable '!' designator: a '!' followed by
   a non-whitespace, non-'=' character and not inside single quotes. The '='
   guard avoids expanding BASH_ENV=!foo style assignments. */
static int	is_bang_expand(const char *input, size_t i)
{
	return (input[i] == '!' && input[i + 1] && input[i + 1] != ' '
		&& input[i + 1] != '\t' && input[i + 1] != '\n'
		&& input[i + 1] != '=' && !in_sq(input, i));
}

/* NUL-terminate and return the expansion result. If it differs from the
   original input, echo it to stdout (bash does the same to let the user see
   what was expanded before running it) -- unless the caller asked for
   silence, which `history -p` does because it prints the result itself and
   prints it even when the expansion changed nothing. */
static char	*finish_expand(t_shell *state, t_string *result, const char *in)
{
	vec_ensure_space_n(result, 1);
	((char *)result->ctx)[result->len] = '\0';
	if (!state->hist.quiet_expand && ft_strcmp((char *)result->ctx, in) != 0)
		ft_printf("%s\n", (char *)result->ctx);
	return ((char *)result->ctx);
}

/* Top-level history expansion entry point: handles ^old^new at the start of
   the line (quick substitution), then walks the rest scanning for '!' events.
   Returns a heap-allocated expanded string (caller must xfree), or NULL when
   history is inactive or input is NULL. If expansion is a no-op the returned
   pointer is a copy of input (not the same pointer). */
char	*expand_history(t_shell *state, const char *input)
{
	t_string	result;
	size_t		i;
	char		*err;

	if (!state->hist.hist_active || !input)
		return (NULL);
	if (input[0] == '^')
		return (quick_sub(state, input));
	vec_init(&result);
	result.elem_size = 1;
	i = 0;
	while (input[i])
	{
		if (is_bang_expand(input, i))
		{
			err = expand_bang_loop(state, &result, input, &i);
			if (err)
				return (err);
		}
		else
			vec_push(&result, (void *)&input[i++]);
	}
	return (finish_expand(state, &result, input));
}
