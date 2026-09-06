/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_utils7.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast_private.h"

/* Third part of the node_name dispatch: the node types added after
   node_name_compound had used up its line budget.  ft_assert(0) is the
   guard the first two parts rely on: every valid t_ast_type must have
   been handled by here. */
char	*node_name_late(t_ast_type tn)
{
	if (tn == AST_SELECT)
		return ("AST_SELECT");
	ft_assert(0);
	return (0);
}
