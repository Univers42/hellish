/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_tokenizer.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"
#include "shell.h"
#include "lexer.h"
#include <stdlib.h>

/*
** Parse a bracket expression [...]
** For [[::]], the content includes the inner [::]
** Returns the length consumed (including brackets), or 0 on error
*/
static int	parse_bracket(const char *s, int max_len, t_glob *g)
{
	int		i;
	int		content_start;
	int		flags;

	if (*s != '[' || max_len < 2)
		return (0);
	i = 1;
	flags = 0;
	/* Check for negation */
	if (i < max_len && (s[i] == '!' || s[i] == '^'))
	{
		flags |= BRACKET_NEGATED;
		i++;
	}
	/* Empty bracket after negation [!] or [] - treat as literal */
	if (i >= max_len || s[i] == ']')
		return (0);
	content_start = i;
	/* First char after [ or [! can be ] without closing the bracket */
	if (s[i] == ']')
		i++;
	/* Find closing bracket, handling nested [: :] */
	while (i < max_len)
	{
		if (s[i] == ']')
			break;
		/* Skip over [:name:] sequences */
		if (s[i] == '[' && i + 1 < max_len && s[i + 1] == ':')
		{
			i += 2;
			while (i + 1 < max_len && !(s[i] == ':' && s[i + 1] == ']'))
				i++;
			if (i + 1 < max_len)
				i += 2;  /* skip :] */
		}
		else
			i++;
	}
	/* If no closing bracket found, treat as literal */
	if (i >= max_len)
		return (0);
	/* Empty content after processing - treat as literal */
	if (i == content_start)
		return (0);
	g->ty = G_BRACKET;
	g->start = s + content_start;
	g->len = i - content_start;
	g->flags = flags;
	g->char_set = glob_expand_bracket(g->start, g->len, &g->char_set_len);
	/* If expansion produced empty set, treat as literal */
	if (!g->char_set || g->char_set_len == 0)
	{
		if (g->char_set)
			free(g->char_set);
		g->char_set = NULL;
		return (0);
	}
	return (i + 1);  /* Include closing ] */
}

/*
** Tokenize asterisk (collapse multiple *'s into one)
*/
static void	tokenize_asterisk(t_vec_glob *ret, const char *s, int *i, int len)
{
	t_glob	g;

	while (*i < len && s[*i] == '*')
		(*i)++;
	g = (t_glob){.ty = G_ASTERISK, .start = s, .len = 1};
	vec_push(ret, &g);
}

/*
** Tokenize question mark
*/
static void	tokenize_question(t_vec_glob *ret, const char *s, int *i)
{
	t_glob	g;

	g = (t_glob){.ty = G_QUESTION, .start = s + *i, .len = 1};
	vec_push(ret, &g);
	(*i)++;
}

/*
** Tokenize literal characters until next special char or end
** If force_one is true, consume at least one character even if special
*/
static void	tokenize_literal_n(t_vec_glob *ret, const char *s, int *i, int len,
							bool can_glob, int force_n)
{
	int		start;
	t_glob	g;

	start = *i;
	/* Force consume at least force_n characters */
	while (*i < len && force_n > 0)
	{
		(*i)++;
		force_n--;
	}
	while (*i < len)
	{
		if (s[*i] == '/')
			break ;
		if (can_glob && (s[*i] == '*' || s[*i] == '?' || s[*i] == '['))
			break ;
		(*i)++;
	}
	if (*i > start)
	{
		g = (t_glob){.ty = G_LITERAL, .start = s + start, .len = *i - start};
		vec_push(ret, &g);
	}
}

/*
** Tokenize literal characters until next special char or end
*/
static void	tokenize_literal(t_vec_glob *ret, const char *s, int *i, int len,
							bool can_glob)
{
	tokenize_literal_n(ret, s, i, len, can_glob, 0);
}

/*
** Main tokenization function
** quoted = true means glob special chars should be treated as literals
*/
t_vec_glob	glob_tokenize(const char *pattern, int len, bool quoted)
{
	t_vec_glob	ret;
	int			i;
	int			consumed;
	t_glob		g;

	vec_init(&ret);
	ret.elem_size = sizeof(t_glob);
	i = 0;
	while (i < len)
	{
		if (pattern[i] == '/')
		{
			g = (t_glob){.ty = G_SLASH, .start = pattern + i, .len = 1};
			vec_push(&ret, &g);
			i++;
		}
		else if (!quoted && pattern[i] == '*')
			tokenize_asterisk(&ret, pattern, &i, len);
		else if (!quoted && pattern[i] == '?')
			tokenize_question(&ret, pattern, &i);
		else if (!quoted && pattern[i] == '[')
		{
			consumed = parse_bracket(pattern + i, len - i, &g);
			if (consumed > 0)
			{
				vec_push(&ret, &g);
				i += consumed;
			}
			else
			{
				/* Bracket parsing failed - treat '[' as literal and advance */
				tokenize_literal_n(&ret, pattern, &i, len, !quoted, 1);
			}
		}
		else
			tokenize_literal(&ret, pattern, &i, len, !quoted);
	}
	return (ret);
}

/*
** Check if token type allows glob expansion
*/
bool	star_expandable(t_tt tt)
{
	if (tt == TT_SQWORD || tt == TT_DQWORD || tt == TT_DQENVVAR)
		return (false);
	if (tt == TT_WORD || tt == TT_ENVVAR)
		return (true);
	return (true);
}

/*
** Tokenize a single token from the AST
*/
static void	tokenize_ast_token(t_vec_glob *ret, t_token t)
{
	bool		can_glob;
	t_vec_glob	sub;
	size_t		j;

	can_glob = star_expandable(t.tt);
	sub = glob_tokenize(t.start, t.len, !can_glob);
	j = 0;
	while (j < sub.len)
	{
		vec_push(ret, &((t_glob *)sub.ctx)[j]);
		j++;
	}
	free(sub.ctx);
}

/*
** Convert AST word node to glob pattern
*/
t_vec_glob	word_to_glob(t_ast_node word)
{
	size_t		i;
	t_ast_node	curr_node;
	t_vec_glob	ret;

	vec_init(&ret);
	ret.elem_size = sizeof(t_glob);
	if (!word.children.ctx)
		return (ret);
	i = 0;
	while (i < word.children.len)
	{
		curr_node = *(t_ast_node *)vec_idx(&word.children, i);
		if (curr_node.node_type != AST_TOKEN)
			return (ret);
		tokenize_ast_token(&ret, curr_node.token);
		i++;
	}
	return (ret);
}
