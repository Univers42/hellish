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
#include "sh_input.h"
#include "decomposer.h"

void	exit_clean(t_shell *state, int code);

/* Expand the word part of a ${name-word} form: tilde, command/arith and
   parameter (incl. nested ${...}) expansion, but no splitting/globbing. */
static char	*expand_param_word(t_shell *state, const char *word, int wlen)
{
	t_ast_node	w;
	t_string	s;
	char		*ret;

	if (wlen <= 0)
		return (ft_strdup(""));
	w = reparse_word((t_token){.start = (char *)word, .len = wlen,
			.tt = TT_WORD});
	expand_tilde_word(state, &w);
	expand_cmd_substitutions(state, &w);
	expand_env_vars(state, &w, false);
	s = word_to_string(w);
	if (!s.ctx)
		ret = ft_strdup("");
	else
		ret = ft_strndup((char *)s.ctx, s.len);
	free(s.ctx);
	free_ast(&w);
	return (ret);
}

typedef struct s_pe_op
{
	const char	*name;
	int			name_len;
	char		opc;
	bool		colon;
	const char	*word;
	int			wlen;
}	t_pe_op;

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
** ${param-word} ${param:-word} ${param+word} ${param:+word}
** ${param=word} ${param:=word} ${param?word} ${param:?word}.
** With a colon, "unset OR null" triggers; without, only "unset" triggers.
*/
static char	*default_or_alt(t_shell *state, char *val, t_pe_op o)
{
	bool	act;

	act = (o.colon) ? is_unset_or_null(val) : (val == NULL);
	if (o.opc == '-')
	{
		if (act)
			return (expand_param_word(state, o.word, o.wlen));
		return (ft_strdup(val));
	}
	if (act)
		return (ft_strdup(""));
	return (expand_param_word(state, o.word, o.wlen));
}

static char	*err_or_assign(t_shell *state, char *val, t_pe_op o)
{
	bool	act;

	act = (o.colon) ? is_unset_or_null(val) : (val == NULL);
	if (o.opc == '?')
	{
		if (!act)
			return (ft_strdup(val));
		ft_eprintf("%s: %.*s: %.*s\n", state->ctx, o.name_len, o.name,
			o.wlen, o.word);
		state->last_cmd_st_exe = create_exec_state(1, false);
		set_cmd_status(state, state->last_cmd_st_exe);
		if (state->metinp != INP_RL)
			exit_clean(state, 1);
		return (ft_strdup(""));
	}
	if (!act)
		return (ft_strdup(val));
	val = expand_param_word(state, o.word, o.wlen);
	env_set(&state->env, env_create(ft_strndup(o.name, o.name_len),
			ft_strdup(val), false));
	return (val);
}

static char	*expand_param_op(t_shell *state, t_pe_op o)
{
	char	*val;

	val = get_var_value(state, o.name, o.name_len);
	if (o.opc == '-' || o.opc == '+')
		return (default_or_alt(state, val, o));
	return (err_or_assign(state, val, o));
}

/* Detect a ${name[:]op word} form (op in -=?+); fill `o`, return true. */
static bool	find_param_op(const char *s, int slen, t_pe_op *o)
{
	int	i;

	i = 0;
	if (i < slen && ft_isdigit((unsigned char)s[i]))
		while (i < slen && ft_isdigit((unsigned char)s[i]))
			i++;
	else if (i < slen && (s[i] == '_' || ft_isalpha((unsigned char)s[i])))
	{
		i++;
		while (i < slen && (s[i] == '_' || ft_isalnum((unsigned char)s[i])))
			i++;
	}
	else
		return (false);
	o->name = s;
	o->name_len = i;
	o->colon = (i < slen && s[i] == ':');
	i += o->colon;
	if (i >= slen || !ft_strchr("-=?+", s[i]))
		return (false);
	o->opc = s[i];
	o->word = s + i + 1;
	o->wlen = slen - i - 1;
	return (true);
}

/*
** Find suffix/prefix removal operators: %, %%, #, ##
** Returns pointer to the operator, or NULL if not found.
*/
static const char	*find_trim_op(const char *s, int slen, int *name_len)
{
	int	i;

	i = 0;
	if (i < slen && ft_isdigit((unsigned char)s[i]))
		while (i < slen && ft_isdigit((unsigned char)s[i]))
			i++;
	else if (i < slen && (s[i] == '_' || ft_isalpha((unsigned char)s[i])))
	{
		i++;
		while (i < slen && (s[i] == '_' || ft_isalnum((unsigned char)s[i])))
			i++;
	}
	else
		return (NULL);
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
	pat = expand_param_word(state, op + op_off, slen - name_len - op_off);
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
	t_pe_op		o;

	if (slen <= 0)
		return (ft_strdup(""));
	if (s[0] == '#' && slen > 1)
		return (expand_strlen(state, s + 1, slen - 1));
	if (find_param_op(s, slen, &o))
		return (expand_param_op(state, o));
	op = find_trim_op(s, slen, &name_len);
	if (op)
		return (expand_trim(state, s, name_len, op, slen));
	return (NULL);
}
