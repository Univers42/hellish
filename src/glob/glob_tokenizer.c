/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_tokenizer.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:43 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:43 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft/libft.h"
#include "shell.h"
# include "glob.h"
# include "lexer.h"

void	tokenize_star_glob(t_vec_glob *ret, t_token t, int *i)
{
	while (*i < t.len && t.start[*i] == '*')
		(*i)++;
	t_glob g = {.ty = G_ASTERISK};
	vec_push(ret, &g);
}

bool	star_expandable(t_tt tt)
{
	if (tt == TT_SQWORD || tt == TT_DQWORD || tt == TT_DQENVVAR)
		return (false);
	else if (tt == TT_WORD || tt == TT_ENVVAR)
		return (true);
	else
	{
		ft_printf("got unexpected: %s\n", tt_to_str(tt));
		ft_assert("Unreachable" == 0);
	}
	return (true);
}

void	tokenize_pattern(t_vec_glob *ret, t_token t, int *i)
{
	bool	expandable;
	char	*start;

	expandable = star_expandable(t.tt);
	start = t.start + *i;
	while (*i < t.len && t.start[*i] != '/'
		&& (t.start[*i] != '*' || !expandable))
		(*i)++;
	t_glob g = {
			.ty = G_PATTERN,
			.start = start,
			.len = t.start + *i - start
	};
	//t_glob	g = create_glob(G_PATTERN, start, t.start + *i - start);
	vec_push(ret, &g);
}

void	tokenize_word_glob(t_vec_glob *ret, t_token t)
{
	int	i;

	i = 0;
	while (i < t.len)
	{
		if (t.start[i] == '/')
		{
			t_glob g = {.ty = G_SLASH};
			vec_push(ret, &g);
			i++;
		}
		else if (t.start[i] == '*' && star_expandable(t.tt))
			tokenize_star_glob(ret, t, &i);
		else
			tokenize_pattern(ret, t, &i);
	}
}

t_vec_glob	word_to_glob(t_ast_node word)
{
	size_t		i;
	t_ast_node	curr_node;
	t_token		curr_t;
	t_vec_glob	ret;

	i = 0;
	vec_init(&ret);
	ret.elem_size = sizeof(t_glob);
	if (!word.children.ctx)
		return ret;
	while (i < word.children.len)
	{
		curr_node = *(t_ast_node *)vec_idx(&word.children, i);
		if (curr_node.node_type != AST_TOKEN)
			return ret;
		ft_assert(curr_node.node_type == AST_TOKEN);
		curr_t = curr_node.token;
		tokenize_word_glob(&ret, curr_t);
		i++;
	}
	return (ret);
}
