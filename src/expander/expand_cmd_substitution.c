/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_cmd_substitution.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 01:34:38 by marvin            #+#    #+#             */
/*   Updated: 2026/01/23 01:34:38 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Walk the AST node tree and expand $(...) and $((...)) in TT_WORD tokens.
   Only TT_WORD subtokens can contain raw $(...) text — TT_ENVVAR already
   has the name extracted and TT_DQWORD / TT_SQWORD are handled by the
   lexer.  The recursion is needed because command substitutions can appear
   inside compound nodes (e.g., a redirect target inside a for-loop body). */
void	expand_cmd_substitutions(t_shell *state, t_ast_node *node)
{
	size_t		i;
	t_ast_node	*ch;
	t_token		*tok;

	if (!node || !node->children.ctx)
		return ;
	i = -1;
	while (++i < node->children.len)
	{
		ch = &((t_ast_node *)node->children.ctx)[i];
		if (ch->node_type == AST_TOKEN)
		{
			tok = &ch->token;
			if (tok->tt == TT_WORD)
				process_word_token(state, tok);
		}
		expand_cmd_substitutions(state, &((t_ast_node *)node->children.ctx)[i]);
	}
}
