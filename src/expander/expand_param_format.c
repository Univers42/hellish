/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

static char	*get_var_value(t_shell *state, const char *name, int len)
{
	return (env_expand_n(state, (char *)name, len));
}

static bool	is_unset_or_null(const char *val)
{
	return (val == NULL || *val == '\0');
}

/*
** Handle ${#parameter} — string length
** Returns the length of the value as a string, or "0" if unset.
*/
static char	*expand_strlen(t_shell *state, const char *s, int slen)
{
	char	*val;
	char	*result;

	val = get_var_value(state, s, slen);
	if (!val)
		return (ft_strdup("0"));
	result = ft_itoa(ft_strlen(val));
	return (result);
}

/*
** Handle colon operators: ${param:-word}, ${param:=word},
** ${param:?word}, ${param:+word}
** 'op' points to ':-', ':=', ':?', or ':+'.
** name_len is the length of the parameter name.
*/
static char	*colon_default_or_alt(char *val, const char *op, int wlen)
{
	if (op[1] == '-')
	{
		if (is_unset_or_null(val))
			return (ft_strndup(op + 2, wlen));
		return (ft_strdup(val));
	}
	if (is_unset_or_null(val))
		return (ft_strdup(""));
	return (ft_strndup(op + 2, wlen));
}

static char	*colon_err_or_assign(t_shell *state, char *val,
	const char *name, const char *op, int name_len, int wlen)
{
	if (op[1] == '?')
	{
		if (is_unset_or_null(val))
		{
			ft_eprintf("%s: %.*s: %.*s\n", state->ctx,
				name_len, name, wlen, op + 2);
			state->last_cmd_st_exe = create_exec_state(1, false);
			set_cmd_status(state, state->last_cmd_st_exe);
			return (ft_strdup(""));
		}
		return (ft_strdup(val));
	}
	if (is_unset_or_null(val))
	{
		env_set(&state->env, env_create(
				ft_strndup(name, name_len), ft_strndup(op + 2, wlen), false));
		return (ft_strndup(op + 2, wlen));
	}
	return (ft_strdup(val));
}

static char	*expand_colon_op(t_shell *state, const char *name,
	int name_len, const char *op, int slen)
{
	char	*val;
	int		wlen;

	val = get_var_value(state, name, name_len);
	wlen = slen - name_len - 2;
	if (op[0] == ':' && (op[1] == '-' || op[1] == '+'))
		return (colon_default_or_alt(val, op, wlen));
	if (op[0] == ':' && (op[1] == '?' || op[1] == '='))
		return (colon_err_or_assign(state, val, name, op, name_len, wlen));
	return (NULL);
}

/*
** Find the first occurrence of a colon operator (:-, :=, :?, :+)
** within the brace content. Returns pointer to the ':' or NULL.
*/
static const char	*find_colon_op(const char *s, int slen)
{
	int	i;

	i = 0;
	if (i < slen && (s[i] == '_' || ft_isalpha((unsigned char)s[i])))
		i++;
	else
		return (NULL);
	while (i < slen && (s[i] == '_' || ft_isalnum((unsigned char)s[i])))
		i++;
	if (i < slen && s[i] == ':' && i + 1 < slen
		&& (s[i + 1] == '-' || s[i + 1] == '='
			|| s[i + 1] == '?' || s[i + 1] == '+'))
		return (&s[i]);
	return (NULL);
}

/*
** Find suffix/prefix removal operators: %, %%, #, ##
** Returns pointer to the operator, or NULL if not found.
*/
static const char	*find_trim_op(const char *s, int slen, int *name_len)
{
	int	i;

	i = 0;
	if (i < slen && (s[i] == '_' || ft_isalpha((unsigned char)s[i])))
		i++;
	else
		return (NULL);
	while (i < slen && (s[i] == '_' || ft_isalnum((unsigned char)s[i])))
		i++;
	*name_len = i;
	if (i < slen && (s[i] == '%' || s[i] == '#'))
		return (&s[i]);
	return (NULL);
}

static bool	pat_match(const char *p, const char *s)
{
	if (*p == '\0')
		return (*s == '\0');
	if (*p == '*')
	{
		while (*p == '*')
			p++;
		if (*p == '\0')
			return (true);
		while (*s)
		{
			if (pat_match(p, s))
				return (true);
			s++;
		}
		return (pat_match(p, s));
	}
	if (*p == '?' && *s != '\0')
		return (pat_match(p + 1, s + 1));
	if (*p == *s && *s != '\0')
		return (pat_match(p + 1, s + 1));
	return (false);
}

static char	*trim_suffix_shortest(const char *val, const char *pattern)
{
	int		vlen;
	int		i;

	vlen = ft_strlen(val);
	i = vlen;
	while (i >= 0)
	{
		if (pat_match(pattern, val + i))
			return (ft_strndup(val, i));
		i--;
	}
	return (ft_strdup(val));
}

static char	*trim_suffix_longest(const char *val, const char *pattern)
{
	int	i;

	i = 0;
	while (val[i])
	{
		if (pat_match(pattern, val + i))
			return (ft_strndup(val, i));
		i++;
	}
	return (ft_strdup(val));
}

static char	*trim_prefix_shortest(const char *val, const char *pattern)
{
	int		vlen;
	int		i;
	char	*sub;

	vlen = ft_strlen(val);
	i = 0;
	while (i <= vlen)
	{
		sub = ft_strndup(val, i);
		if (pat_match(pattern, sub))
		{
			free(sub);
			return (ft_strdup(val + i));
		}
		free(sub);
		i++;
	}
	return (ft_strdup(val));
}

static char	*trim_prefix_longest(const char *val, const char *pattern)
{
	int		vlen;
	int		i;
	char	*sub;

	vlen = ft_strlen(val);
	i = vlen;
	while (i >= 0)
	{
		sub = ft_strndup(val, i);
		if (pat_match(pattern, sub))
		{
			free(sub);
			return (ft_strdup(val + i));
		}
		free(sub);
		i--;
	}
	return (ft_strdup(val));
}

static char	*expand_trim(t_shell *state, const char *name,
	int name_len, const char *op, int slen)
{
	char	*val;
	char	*pat;
	char	*result;
	int		op_off;

	val = get_var_value(state, name, name_len);
	if (!val)
		return (ft_strdup(""));
	op_off = 1 + (op[1] == '%' || op[1] == '#');
	pat = ft_strndup(op + op_off, slen - name_len - op_off);
	if (op[0] == '%' && op[1] == '%')
		result = trim_suffix_longest(val, pat);
	else if (op[0] == '%')
		result = trim_suffix_shortest(val, pat);
	else if (op[0] == '#' && op[1] == '#')
		result = trim_prefix_longest(val, pat);
	else if (op[0] == '#')
		result = trim_prefix_shortest(val, pat);
	else
		result = ft_strdup(val);
	return (free(pat), result);
}

/*
** Main entry: expand a parameter format token.
** The token->start points to the content between ${ and }.
** e.g. for ${HOME:-/tmp}, start="HOME:-/tmp", len=10
** Returns allocated string, or NULL if not a special format (simple var).
*/
char	*expand_param_format(t_shell *state, const char *s, int slen)
{
	const char	*op;
	int			name_len;

	if (slen <= 0)
		return (ft_strdup(""));
	if (s[0] == '#' && slen > 1)
		return (expand_strlen(state, s + 1, slen - 1));
	op = find_colon_op(s, slen);
	if (op)
	{
		name_len = (int)(op - s);
		return (expand_colon_op(state, s, name_len, op, slen));
	}
	op = find_trim_op(s, slen, &name_len);
	if (op)
		return (expand_trim(state, s, name_len, op, slen));
	return (NULL);
}
