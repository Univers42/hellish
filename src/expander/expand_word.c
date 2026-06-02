/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "brace_expand.h"
#include "decomposer.h"
#include "sys.h"

/* A word participates in brace expansion only if it is entirely unquoted
   (all TT_WORD subtokens); quotes/vars inhibit it, matching POSIX shells. */
static bool	word_all_unquoted(t_ast_node *node)
{
	size_t		i;
	int			j;
	t_ast_node	*c;
	bool		has_brace;

	if (!node->children.ctx || node->children.len == 0)
		return (false);
	has_brace = false;
	i = 0;
	while (i < node->children.len)
	{
		c = &((t_ast_node *)node->children.ctx)[i];
		if (c->node_type != AST_TOKEN || c->token.tt != TT_WORD)
			return (false);
		j = 0;
		while (j < c->token.len)
			if (c->token.start[j++] == '{')
				has_brace = true;
		i++;
	}
	return (has_brace);
}

/* Brace-expand `node` into multiple words, re-lexing each result and running
   it through the full expansion pipeline. Returns true if it handled `node`. */
static bool	try_brace_expand(t_shell *state, t_ast_node *node, t_vec *args)
{
	t_string	flat;
	char		*fs;
	t_vec		results;
	t_ast_node	w;
	size_t		i;

	if (!word_all_unquoted(node))
		return (false);
	flat = word_to_string(*node);
	fs = ft_strndup((char *)flat.ctx, flat.len);
	free(flat.ctx);
	if (brace_find_expandable(fs, &(int){0}) < 0)
		return (free(fs), false);
	results = brace_expand_str(fs);
	free(fs);
	i = 0;
	while (i < results.len)
	{
		fs = ((char **)results.ctx)[i++];
		w = reparse_word((t_token){.start = fs, .len = (int)ft_strlen(fs),
				.tt = TT_WORD});
		expand_word(state, &w, args, false);
		free(fs);
	}
	return (free(results.ctx), free_ast(node), true);
}

void	expand_word(t_shell *state, t_ast_node *node,
					t_vec *args, bool keep_as_one)
{
	t_vec_nd	words;
	size_t		i;

	if (!keep_as_one && try_brace_expand(state, node, args))
		return ;
	if (!node->children.ctx || node->children.len == 0)
	{
		vec_push(args, &(char *){ft_strdup("")});
		return (free_ast(node));
	}
	(expand_tilde_word(state, node), expand_cmd_substitutions(state, node));
	(expand_env_vars(state, node, !keep_as_one), vec_init(&words));
	words.elem_size = sizeof(t_ast_node);
	if (!keep_as_one)
		words = split_words(state, node);
	else
	{
		vec_push(&words, node);
		*node = (t_ast_node){};
	}
	i = -1;
	while (++i < words.len)
	{
		expand_node_glob(&((t_ast_node *)words.ctx)[i], args, keep_as_one);
		if (get_g_sig()->should_unwind)
			while (i < words.len)
				free_ast(&((t_ast_node *)words.ctx)[i++]);
		if (get_g_sig()->should_unwind)
			break ;
	}
	(free(words.ctx), free_ast(node));
	return ;
}

char	*expand_proc_sub(t_shell *state, t_ast_node *node)
{
	t_token		*tok;
	t_ast_node	*cmd_word;
	t_string	cmd_str;
	char		*result;
	bool		is_input;

	if (!node || node->node_type != AST_PROC_SUB || node->children.len < 2)
		return (NULL);
	tok = &((t_ast_node *)node->children.ctx)[0].token;
	cmd_word = &((t_ast_node *)node->children.ctx)[1];
	is_input = (tok->tt == TT_PROC_SUB_IN);
	cmd_str = word_to_string(*cmd_word);
	if (!cmd_str.ctx)
		return (ft_strdup(BLACK_HOLE));
	if (!vec_ensure_space_n(&cmd_str, 1))
		return (free(cmd_str.ctx), NULL);
	((char *)cmd_str.ctx)[cmd_str.len] = '\0';
	if (is_input)
		result = create_procsub_input(state, (char *)cmd_str.ctx);
	else
		result = create_procsub_output(state, (char *)cmd_str.ctx);
	free(cmd_str.ctx);
	return (result);
}
