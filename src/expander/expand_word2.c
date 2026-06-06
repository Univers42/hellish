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

/* True when s[0..len) is a valid bare variable name ([A-Za-z_][A-Za-z0-9_]*).
   Used to gate the fast-path: $? $@ $# $1 and ${...} forms all fail this
   check and must go through the general expand_token path. */
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

/* Quick check: does the expanded value need IFS splitting or globbing?
   With the default IFS (" \t\n"), any whitespace means splitting; *, ?, [
   mean glob candidates.  If neither fires, the value is safe to use as-is
   without copying it through the full pipeline. */
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

/* Return the one non-empty token child of `node`, or NULL if the word has
   zero, more than one, or a non-token child.  Empty TT_WORD pieces (from
   adjacent quotes like a""b) are harmless padding and are skipped.  Used
   by the fast-path to detect words like $HOME that are a single $var. */
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

/* Fast path for the extremely common case `$PLAIN_VAR` (one TT_ENVVAR token,
   plain name, default IFS, value has no split/glob chars).  Returns the
   env value directly (caller must NOT free it — borrowed pointer) or NULL
   to fall back to the slow path.  The IFS check matters: a non-default IFS
   could split even a simple value and must use the full pipeline. */
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

/* True when s[0..len) has no character that triggers any shell expansion,
   quoting, or field splitting.  Used by concat_one_token to decide whether
   a TT_WORD or TT_DQWORD slice can be appended verbatim to the output. */
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
