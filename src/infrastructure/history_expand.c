/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_expand.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "history.h"
#include "libft.h"
#include <stdlib.h>

static char	*hist_entry_at(t_shell *state, int idx)
{
	t_vec	*cmds;

	cmds = &state->hist.hist_cmds;
	if (idx < 0)
		idx = (int)cmds->len + idx;
	if (idx < 0 || idx >= (int)cmds->len)
		return (NULL);
	return (((char **)cmds->ctx)[idx]);
}

static char	*hist_last(t_shell *state)
{
	return (hist_entry_at(state, -1));
}

static char	*hist_search_prefix(t_shell *state, const char *prefix, int len)
{
	int	i;

	i = (int)state->hist.hist_cmds.len - 1;
	while (i >= 0)
	{
		if (ft_strncmp(((char **)state->hist.hist_cmds.ctx)[i],
				prefix, len) == 0)
			return (((char **)state->hist.hist_cmds.ctx)[i]);
		i--;
	}
	return (NULL);
}

static char	*resolve_bang(t_shell *state, const char *s, size_t *adv)
{
	int		num;
	int		neg;
	size_t	wlen;

	if (s[0] == '!')
		return (*adv = 1, hist_last(state));
	if (s[0] == '-' && s[1] >= '0' && s[1] <= '9')
	{
		neg = 1;
		num = 0;
		while (s[neg] >= '0' && s[neg] <= '9')
			num = num * 10 + (s[neg++] - '0');
		*adv = neg;
		return (hist_entry_at(state, -num));
	}
	if (s[0] >= '0' && s[0] <= '9')
	{
		num = 0;
		neg = 0;
		while (s[neg] >= '0' && s[neg] <= '9')
			num = num * 10 + (s[neg++] - '0');
		*adv = neg;
		return (hist_entry_at(state, num - 1));
	}
	wlen = 0;
	while (s[wlen] && s[wlen] != ' ' && s[wlen] != '\t' && s[wlen] != '\n')
		wlen++;
	*adv = wlen;
	return (hist_search_prefix(state, s, wlen));
}

static int	in_single_quote(const char *s, size_t pos)
{
	size_t	i;
	int		sq;

	sq = 0;
	i = 0;
	while (i < pos)
	{
		if (s[i] == '\'')
			sq = !sq;
		i++;
	}
	return (sq);
}

char	*expand_history(t_shell *state, const char *input)
{
	t_string	result;
	size_t		i;
	size_t		adv;
	char		*sub;

	if (!state->hist.hist_active || !input)
		return (NULL);
	vec_init(&result);
	result.elem_size = 1;
	i = 0;
	while (input[i])
	{
		if (input[i] == '!' && input[i + 1] && input[i + 1] != ' '
			&& input[i + 1] != '\t' && input[i + 1] != '\n'
			&& input[i + 1] != '=' && !in_single_quote(input, i))
		{
			adv = 0;
			sub = resolve_bang(state, input + i + 1, &adv);
			if (!sub)
			{
				ft_eprintf("%s: !%.*s: event not found\n",
					state->ctx, (int)adv, input + i + 1);
				free(result.ctx);
				return (ft_strdup(""));
			}
			vec_push_str(&result, sub);
			i += 1 + adv;
		}
		else
			vec_push(&result, (void *)&input[i++]);
	}
	vec_ensure_space_n(&result, 1);
	((char *)result.ctx)[result.len] = '\0';
	if (ft_strcmp((char *)result.ctx, input) != 0)
		ft_printf("%s\n", (char *)result.ctx);
	return ((char *)result.ctx);
}
