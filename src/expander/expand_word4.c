/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word4.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

void	expand_word(t_shell *state, t_ast_node *node,
			t_vec *args, bool keep_as_one)
{
	int	flags;

	flags = 0;
	if (keep_as_one)
		flags = EW_KEEP_AS_ONE;
	expand_word_glob_ctl(state, node, args, flags);
}

static void	glob_loop(t_expand_glob_ctx *gctx, t_vec *words)
{
	size_t	i;

	i = -1;
	while (++i < words->len)
	{
		expand_node_glob(&((t_ast_node *)words->ctx)[i], gctx->args,
			gctx->keep_as_one, gctx->no_glob);
		if (get_g_sig()->should_unwind)
			while (i < words->len)
				free_ast(&((t_ast_node *)words->ctx)[i++]);
		if (get_g_sig()->should_unwind)
			break ;
	}
}

static t_vec_nd	build_words(t_shell *state, t_ast_node *node,
					bool keep_as_one)
{
	t_vec_nd	words;

	(expand_tilde_word(state, node), expand_cmd_substitutions(state, node));
	(expand_env_vars(state, node, !keep_as_one), vec_init(&words));
	words.elem_size = sizeof(t_ast_node);
	if (!keep_as_one)
		return (split_words(state, node));
	vec_push(&words, node);
	*node = (t_ast_node){};
	return (words);
}

void	expand_word_glob_ctl(t_shell *state, t_ast_node *node,
			t_vec *args, int flags)
{
	t_vec_nd			words;
	t_expand_glob_ctx	gctx;
	bool				keep_as_one;
	bool				no_glob;

	keep_as_one = (flags & EW_KEEP_AS_ONE) != 0;
	no_glob = (flags & EW_NO_GLOB) != 0;
	if (state->opt_noglob)
		no_glob = true;
	if (!keep_as_one && try_brace_expand(state, node, args))
		return ;
	if (!node->children.ctx || node->children.len == 0)
	{
		vec_push(args, &(char *){ft_strdup("")});
		return (free_ast(node));
	}
	words = build_words(state, node, keep_as_one);
	gctx.state = state;
	gctx.args = args;
	gctx.keep_as_one = keep_as_one;
	gctx.no_glob = no_glob;
	glob_loop(&gctx, &words);
	(free(words.ctx), free_ast(node));
}
