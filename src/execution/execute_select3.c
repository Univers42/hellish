/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_select3.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include <stdlib.h>
#include <wchar.h>

/* Display width of a word: wcswidth in a multibyte locale, bytes
   otherwise -- bash's displen(). */
int	sel_displen(const char *s)
{
	wchar_t	*w;
	size_t	n;
	int		r;

	if (MB_CUR_MAX <= 1)
		return ((int)ft_strlen(s));
	n = mbstowcs(NULL, s, 0);
	if (n == (size_t)-1)
		return ((int)ft_strlen(s));
	w = xcalloc(n + 1, sizeof(wchar_t));
	if (!w)
		return ((int)ft_strlen(s));
	mbstowcs(w, s, n + 1);
	r = wcswidth(w, n);
	xfree(w);
	if (r < 0)
		return ((int)ft_strlen(s));
	return (r);
}

/* Digits in n -- bash's NUMBER_LEN. */
int	sel_numlen(int n)
{
	int	d;

	d = 1;
	while (n >= 10)
	{
		n /= 10;
		d++;
	}
	return (d);
}

/* bash's valid_number: optional blanks, an optional sign, digits, blanks
   and nothing else.  The value is capped well past any menu length, so an
   absurdly long digit string stays out of range instead of wrapping. */
bool	sel_number(const char *s, long *n)
{
	bool	neg;

	while (*s == ' ' || *s == '\t')
		s++;
	neg = (*s == '-');
	if (*s == '-' || *s == '+')
		s++;
	if (!ft_isdigit((unsigned char)*s))
		return (false);
	*n = 0;
	while (ft_isdigit((unsigned char)*s))
	{
		if (*n < 1000000000L)
			*n = *n * 10 + (*s - '0');
		s++;
	}
	while (*s == ' ' || *s == '\t')
		s++;
	if (neg)
		*n = -*n;
	return (*s == '\0');
}

/* write(2) with its result acknowledged: the fortified libc marks it
   warn_unused_result, and there is nothing to do about a failed prompt
   write but carry on to the read. */
void	sel_write(int fd, const char *s, size_t n)
{
	if (write(fd, s, n) < 0)
		return ;
}

/* Cell geometry, from bash's print_select_list: cells one width, as many
   columns as fit, rows to hold everything, then the columns recomputed
   from the rows; a single row becomes a single column instead. */
void	sel_geometry(t_selmenu *m, int width)
{
	int	len;
	int	cols;

	len = (int)m->words->len;
	m->idx_len = sel_numlen(len);
	m->max_len += m->idx_len + 4;
	cols = width / m->max_len;
	if (cols == 0)
		cols = 1;
	m->rows = len / cols + (len % cols != 0);
	cols = len / m->rows + (len % m->rows != 0);
	if (m->rows == 1)
	{
		m->rows = cols;
		cols = 1;
	}
	m->first_len = sel_numlen(m->rows);
}
