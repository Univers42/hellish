/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   word_to_pattern.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 12:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/07/22 12:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "expander.h"
#include "decomposer.h"

/* Copy quoted subtoken text with every glob metachar backslash-escaped.
   POSIX quote removal makes a quoted metachar LITERAL, so `${v%2'*'}` must
   match a real '*' rather than globbing -- bash, dash and mksh all agree.
   The escape is consumed by pat_match_pub, which reads `\X` as literal X. */
static void	pat_append_escaped(t_string *s, t_token t)
{
	int	i;

	i = 0;
	while (i < t.len)
	{
		if (t.start[i] == '*' || t.start[i] == '?'
			|| t.start[i] == '[' || t.start[i] == '\\')
			vec_push_char(s, '\\');
		vec_push_char(s, t.start[i]);
		i++;
	}
}

/* One subtoken: quoted text becomes literal (escaped), everything else keeps
   its metachars active so `${v%*.c}` still globs. */
static void	pat_append(t_string *s, t_token t)
{
	if (!t.start)
		return ;
	if (t.tt == TT_SQWORD || t.tt == TT_DQWORD)
		pat_append_escaped(s, t);
	else
		vec_push_nstr(s, t.start, (size_t)t.len);
}

/* Flatten a reparsed word into a PATTERN -- word_to_string's sibling, but
   quote-aware (see pat_append). */
t_string	word_to_pattern(t_ast_node node)
{
	t_string	s;
	size_t		i;
	t_ast_node	*child;

	vec_init(&s);
	s.elem_size = 1;
	if (!node.children.ctx)
		return (s);
	i = -1;
	while (++i < node.children.len)
	{
		child = &((t_ast_node *)node.children.ctx)[i];
		if (child->node_type != AST_TOKEN)
			break ;
		pat_append(&s, child->token);
	}
	return (s);
}

/* expand_param_word's pattern twin: same tilde/cmdsub/var pass, but flattened
   with word_to_pattern so quoted metachars survive as literals. Used by the
   #/% trims and ${v/pat/rep}, never by the :- style word forms (those are
   plain words, not patterns). */
char	*expand_param_pattern(t_shell *state, const char *word, int wlen)
{
	t_ast_node	w;
	t_token		t;
	t_string	s;
	char		*ret;

	if (wlen <= 0)
		return (ft_strdup(""));
	t.start = (char *)word;
	t.len = wlen;
	t.tt = TT_WORD;
	w = reparse_word(t, false);
	expand_tilde_word(state, &w);
	expand_cmd_substitutions(state, &w);
	expand_env_vars(state, &w, false);
	s = word_to_pattern(w);
	if (!s.ctx)
		ret = ft_strdup("");
	else
		ret = ft_strndup((char *)s.ctx, s.len);
	xfree(s.ctx);
	free_ast(&w);
	return (ret);
}
