/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_expand2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 11:11:45 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/25 21:06:56 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

int				is_letter(unsigned char c);
int				is_digit_char(unsigned char c);
int				is_alnum_char(unsigned char c);
unsigned char	to_lower(unsigned char c);
int				ft_strcoll(const char *s1, const char *s2);
void			glob_sort_inner(char **arr, int low, int high);
void			glob_sort(t_vec *args);

/* Return true if a token of type `tt` should have its wildcards expanded.
   Single-quoted words (TT_SQWORD) and double-quoted content (TT_DQWORD,
   TT_DQENVVAR) suppress glob expansion; everything else is expandable.
   This maps the shell quoting rules to a boolean for the tokenizer. */
bool	star_expandable(t_tt tt)
{
	if (tt == TT_SQWORD || tt == TT_DQWORD || tt == TT_DQENVVAR)
		return (false);
	if (tt == TT_WORD || tt == TT_ENVVAR)
		return (true);
	return (true);
}

/* Convert an AST word node (a list of token children) into a flat t_vec_glob.
   Each child token is individually tokenized via tokenize_ast_token, which
   respects the quoting of each sub-token -- so "foo*" produces one literal
   token "foo*", but unquoted foo* produces a G_LITERAL "foo" and
   a G_ASTERISK. */
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

/* Scan the token list for any wildcard (*, ?, bracket). A word with no
   wildcards can never expand to more than itself, so we skip the opendir call
   entirely. This is the single biggest performance win for completion and
   heavy-argument commands: no filesystem I/O for plain strings. */
static bool	glob_has_wildcard(t_vec_glob glob)
{
	size_t	i;
	t_glob	*g;

	i = 0;
	g = (t_glob *)glob.ctx;
	while (i < glob.len)
	{
		if (g[i].ty == G_ASTERISK || g[i].ty == G_QUESTION
			|| g[i].ty == G_BRACKET)
			return (true);
		i++;
	}
	return (false);
}

/* Expand a word node by glob matching and return a vector of malloc'd path
   strings. If the pattern has no wildcards, the word is returned as-is
   (after converting to string) without any filesystem access. If wildcards
   exist, we start the directory walk at "/" for absolute patterns (first token
   is G_SLASH) or at "" (meaning CWD) for relative ones. When the walk finds
   nothing, the original word is pushed unchanged -- POSIX "no-match = literal".
   Results are sorted with glob_sort after the walk. If a signal arrived during
   the walk (should_unwind), we destroy the partial results and return empty. */
t_vec	expand_word_glob(t_ast_node word)
{
	t_vec		args;
	t_vec_glob	glob;

	vec_init(&args);
	args.elem_size = sizeof(char *);
	glob = word_to_glob(word);
	if (glob.len == 0)
		return (vec_push(&args, &(char *){ft_strdup("")}), args);
	if (!glob_has_wildcard(glob))
	{
		vec_push(&args, &(char *){(char *)word_to_string(word).ctx});
		return (glob_free_tokens(&glob), args);
	}
	if (((t_glob *)glob.ctx)[0].ty == G_SLASH)
		match_dir(&args, glob, "/", 1);
	else
		match_dir(&args, glob, "", 0);
	if (args.len == 0)
		vec_push(&args, &(char *){(char *)word_to_string(word).ctx});
	glob_free_tokens(&glob);
	if (!get_g_sig()->should_unwind)
		glob_sort(&args);
	if (get_g_sig()->should_unwind)
		vec_destroy(&args, free_str_elem);
	return (args);
}
