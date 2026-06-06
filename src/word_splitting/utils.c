/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 21:49:33 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/26 16:13:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Carve a subtoken out of token t using [offset.start, offset.end) and push
   it as a new child of ret. The child's token type is set to tt so the
   expander knows what to do with it (TT_SQWORD = literal, TT_ENVVAR = expand,
   TT_DQWORD = literal-no-split, etc.). children.elem_size is set on the temp
   so vec_push copies the right number of bytes. This is the single routine
   all reparse helpers funnel through -- one place to add tracing or asserts. */
void	push_subtoken_node(t_ast_node *ret,
			t_token t,
			t_interval offset,
			t_tt tt)
{
	t_ast_node	tmp;

	tmp = create_subtoken_node(t, offset.start, offset.end, tt);
	tmp.children.elem_size = sizeof(t_ast_node);
	vec_push(&ret->children, &tmp);
}

/* Return true iff the first `len` bytes of s form a valid POSIX identifier:
   first char [a-zA-Z_], remaining chars [a-zA-Z0-9_]. Used both to validate
   the key side of a KEY=val assignment and to check if a ${...} expansion
   body is a plain name. len <= 0 or a non-ident first char short-circuits
   immediately to avoid reading past the end of the token. */
bool	is_valid_ident(char *s, int len)
{
	int			i;

	i = 0;
	if (len <= 0 || !is_var_name_p1(s[0]))
		return (false);
	while (i < len)
	{
		if (!is_var_name_p2(s[i++]))
			return (false);
	}
	return (true);
}
