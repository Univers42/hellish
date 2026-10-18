/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_provider2.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Word-walking helpers shared by the completion providers. */

#include "complete_make.h"

/* The next blank-separated word before the cursor word, 0 at the end. */
size_t	prov_word(t_pv_line *p, const char **w)
{
	size_t	n;

	while (p->pos < p->end && mk_blank(p->l[p->pos]))
		p->pos++;
	*w = p->l + p->pos;
	n = 0;
	while (p->pos + n < p->end && !mk_blank(p->l[p->pos + n]))
		n++;
	p->pos += n;
	return (n);
}

/* Offset of the basename in w[0..n), so /usr/bin/make is make. */
size_t	prov_base(const char *w, size_t n)
{
	size_t	i;

	i = n;
	while (i > 0 && w[i - 1] != '/')
		i--;
	return (i);
}
