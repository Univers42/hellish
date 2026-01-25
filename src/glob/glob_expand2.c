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

/*
** Expand a word node with glob matching
** Returns vector of matched filenames (or original word if no matches)
*/
t_vec	expand_word_glob(t_ast_node word)
{
	t_vec		args;
	t_vec_glob	glob;

	vec_init(&args);
	args.elem_size = sizeof(char *);
	glob = word_to_glob(word);
	if (glob.len == 0)
		return (vec_push(&args, &(char *){ft_strdup("")}), args);
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
