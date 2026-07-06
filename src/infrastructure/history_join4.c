/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_join4.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"
#include "libft.h"

/* Zero the joiner state and set up its two growable buffers (the joined
   output text and the queue of pending here-doc tags). */
void	hj_init(t_hjoin *h, const char *cmd)
{
	ft_memset(h, 0, sizeof(*h));
	h->s = cmd;
	vec_init(&h->out);
	h->out.elem_size = 1;
	vec_init(&h->tags);
	h->tags.elem_size = sizeof(char *);
}

/* True at a << that can open a here-doc (not quoted, not arithmetic). */
bool	hj_at_heredoc(t_hjoin *h)
{
	return (h->s[h->i] == '<' && h->s[h->i + 1] == '<'
		&& !h->sq && !h->dq && h->arith == 0);
}

/* Consume a $(( / $( / ${ opener atomically (so its parens or brace are
   not double-counted by the per-char tracking) and bump the matching
   depth counter.  Returns false when the '$' opens nothing. */
bool	hj_dollar(t_hjoin *h)
{
	char	c;

	c = h->s[h->i + 1];
	if (c != '(' && c != '{')
		return (false);
	h->cpat = false;
	if (c == '(' && h->s[h->i + 2] == '(')
	{
		h->arith += 2;
		vec_push_nstr(&h->out, (char *)h->s + h->i, 3);
		h->i += 3;
		return (true);
	}
	if (c == '(')
		h->csub++;
	else
		h->dpar++;
	vec_push_nstr(&h->out, (char *)h->s + h->i, 2);
	h->i += 2;
	return (true);
}
