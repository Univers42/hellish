/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_tilde_word.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:31:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:31:22 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

void	expand_tilde_word(t_shell *state, t_ast_node *curr)
{
	t_token	*first;
	bool	should_expand;

	ft_assert(curr->children.len);
	if (((t_ast_node *)curr->children.ctx)[0].token.tt != TT_WORD)
		return ;
	first = &((t_ast_node *)curr->children.ctx)[0].token;
	should_expand = false;
	should_expand |= token_starts_with(*first, "~")
		&& curr->children.len == 1 && first->len == 1;
	should_expand |= token_starts_with(*first, "~/");
	should_expand |= token_starts_with(*first, "~+")
		&& curr->children.len == 1 && first->len == 2;
	should_expand |= token_starts_with(*first, "~+/");
	should_expand |= token_starts_with(*first, "~-")
		&& curr->children.len == 1 && first->len == 2;
	should_expand |= token_starts_with(*first, "~-/");
	if (should_expand)
		expand_tilde_token(state, first);
}
