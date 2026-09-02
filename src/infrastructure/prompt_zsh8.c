/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_zsh8.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"

/* zsh's PROMPT_SUBST order -- the other half of issue #112.
**
** zsh substitutes parameters FIRST and then expands prompt escapes over
** the result, which is what lets every vcs_info tutorial write
**
**     zstyle ':vcs_info:git:*' formats '%F{magenta}%b%f'
**     PROMPT='%~ ${vcs_info_msg_0_} %# '
**
** and get a coloured branch: the %F inside the message is expanded by the
** prompt, not by vcs_info. Here the reader ran the other way round -- the
** zsh->backslash conversion happened on the format text, and `${...}` was
** left for the backslash renderer to fill in later -- so the message's
** escapes arrived after the only pass that understood them and printed as
** `%F{242}on%f`. Under exact zsh semantics (strict), a simple `$NAME` or
** `${NAME}` is therefore substituted at conversion time and its value run
** through the same converter. One level only, as zsh does: the value's
** own dollars are text. Anything richer (`${x:-y}`, `$(...)`, specials)
** keeps the old route through the backslash renderer.
*/
static size_t	param_name(const char *s, char *name, size_t cap)
{
	size_t	i;
	size_t	n;
	bool	braced;

	i = 1;
	braced = (s[i] == '{');
	if (braced)
		i++;
	n = 0;
	if (!(ft_isalpha(s[i]) || s[i] == '_'))
		return (0);
	while ((ft_isalnum(s[i]) || s[i] == '_') && n + 1 < cap)
		name[n++] = s[i++];
	name[n] = '\0';
	if (braced && s[i] != '}')
		return (0);
	return (i + braced);
}

bool	zsh_param_subst(t_shell *state, t_string *out, const char *fmt,
			int *i)
{
	static int	depth;
	char		name[128];
	size_t		used;
	char		*val;
	t_string	conv;

	if (depth > 0 || fmt[*i] != '$')
		return (false);
	used = param_name(fmt + *i, name, sizeof(name));
	if (!used)
		return (false);
	val = env_expand(state, name);
	*i += (int)used;
	if (!val || !*val)
		return (true);
	depth++;
	conv = zsh_to_ps1(state, val, true);
	depth--;
	vec_push_str(out, (char *)conv.ctx);
	xfree(conv.ctx);
	return (true);
}
