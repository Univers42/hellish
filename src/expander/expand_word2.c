/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

bool	name_is_plain(const char *s, int len)
{
	int	i;

	if (len <= 0 || !(ft_isalpha(s[0]) || s[0] == '_'))
		return (false);
	i = 1;
	while (i < len)
	{
		if (!(ft_isalnum(s[i]) || s[i] == '_'))
			return (false);
		i++;
	}
	return (true);
}

/* Does the value contain a char that would trigger field-splitting (default
   IFS) or pathname expansion? */
bool	needs_split_or_glob(const char *v)
{
	while (*v)
	{
		if (*v == ' ' || *v == '\t' || *v == '\n'
			|| *v == '*' || *v == '?' || *v == '[')
			return (true);
		v++;
	}
	return (false);
}

/* The single non-empty sub-token of a word, skipping empty TT_WORD pieces. */
t_token	*lone_nonempty_token(t_ast_node *node)
{
	t_token		*t;
	t_ast_node	*c;
	int			i;

	t = NULL;
	i = 0;
	while (i < (int)node->children.len)
	{
		c = &((t_ast_node *)node->children.ctx)[i++];
		if (c->node_type != AST_TOKEN)
			return (NULL);
		if (c->token.tt == TT_WORD && c->token.len == 0)
			continue ;
		if (t)
			return (NULL);
		t = &c->token;
	}
	return (t);
}

char	*try_simple_envvar(t_shell *state, t_ast_node *node)
{
	t_token	*t;
	char	*ifs;
	char	*v;

	t = lone_nonempty_token(node);
	if (!t || t->tt != TT_ENVVAR || !name_is_plain(t->start, t->len))
		return (NULL);
	ifs = env_get_ifs(&state->env);
	if (ifs && ft_strcmp(ifs, " \t\n") != 0)
		return (NULL);
	v = env_expand_n(state, t->start, t->len);
	if (!v || !*v || needs_split_or_glob(v))
		return (NULL);
	return (v);
}

/* Literal text with no character that would need any expansion or quoting. */
bool	is_plain_literal_text(const char *s, int len)
{
	int	i;

	i = 0;
	while (i < len)
	{
		if (ft_strchr("$`\\'\"~{}*?[] \t\n", s[i]))
			return (false);
		i++;
	}
	return (true);
}
