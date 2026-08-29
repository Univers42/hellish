/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh0.c                                      :+:      :+:    :+:   */
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

#include "shell.h"
#include "ft_builtins.h"

char	*expand_special_dyn(t_shell *state, char *key, int len);
char	*build_flagstr(t_shell *state);

/* zsh's $0: inside a function it is the FUNCTION'S NAME, not the shell's.
   bash keeps the shell name, so this answers NULL outside the dialect and
   the caller falls through to whatever $0 already was. It is the mechanism
   behind `function man dman debman { colored $0 "$@" }` -- one body, three
   names, and $0 is how it tells which one ran. */
char	*zsh_arg_zero(t_shell *state)
{
	const char	*fn;

	if (!zsh_mode(state))
		return (NULL);
	fn = frame_func_name(state);
	if (!fn)
		return (NULL);
	return ((char *)fn);
}

/* The one-character specials that are not $? $$ $!: $- , $0 and $#.  Split
   from expand_special only to stay inside the norm line budget; the order
   is unchanged and NULL still means "not one of these". */
char	*expand_special_1(t_shell *state, char c)
{
	if (c == '-')
		return (build_flagstr(state));
	if (c == '0')
		return (zsh_arg_zero(state));
	if (c == '#')
	{
		if (state->pos.cnt_str[0])
			return (state->pos.cnt_str);
		return ("0");
	}
	return (expand_special_dyn(state, &c, 1));
}
