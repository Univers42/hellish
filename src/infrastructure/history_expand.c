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

/* !?str[?] : most recent history entry containing `str`. */
static char	*hist_search_contains(t_shell *state, const char *needle, int len)
{
	int		i;
	char	*nd;
	char	*cmd;

	nd = ft_strndup(needle, len);
	if (!nd)
		return (NULL);
	i = (int)state->hist.hist_cmds.len - 1;
	while (i >= 0)
	{
		cmd = ((char **)state->hist.hist_cmds.ctx)[i];
		if (ft_strnstr(cmd, nd, ft_strlen(cmd)))
			return (free(nd), cmd);
		i--;
	}
	return (free(nd), NULL);
}

static char	*resolve_bang(t_shell *state, const char *s, size_t *adv)
{
	int		num;
	int		neg;
	size_t	wlen;

	if (s[0] == '!')
		return (*adv = 1, hist_last(state));
	if (s[0] == '?')
	{
		wlen = 1;
		while (s[wlen] && s[wlen] != '?' && s[wlen] != '\n')
			wlen++;
		*adv = wlen + (s[wlen] == '?');
		return (hist_search_contains(state, s + 1, (int)wlen - 1));
	}
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

/* last command with the first occurrence of `old` replaced by `new`. */
static char	*replace_first(const char *s, const char *old, const char *nw)
{
	t_string	r;
	char		*occ;

	occ = ft_strnstr(s, old, ft_strlen(s));
	if (!occ || !*old)
		return (NULL);
	vec_init(&r);
	r.elem_size = 1;
	vec_push_nstr(&r, (char *)s, occ - s);
	vec_push_str(&r, (char *)nw);
	vec_push_str(&r, occ + ft_strlen(old));
	return (vec_push(&r, &(char){0}), (char *)r.ctx);
}

/* ^old^new[^] : re-run the last command with old -> new (csh quick sub). */
static char	*quick_sub(t_shell *state, const char *input)
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
	free(old);
	free(nw);
	if (!res)
		return (ft_eprintf("%s: :s: substitution failed\n", state->ctx),
			ft_strdup(""));
	return (ft_printf("%s\n", res), res);
}

char	*expand_history(t_shell *state, const char *input)
{
	t_string	result;
	size_t		i;
	size_t		adv;
	char		*sub;

	if (!state->hist.hist_active || !input)
		return (NULL);
	if (input[0] == '^')
		return (quick_sub(state, input));
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
