/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_select2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "env.h"

int	get_cols(void);

/* The menu, laid out exactly as bash's print_select_list does it: cells
   of one width (the widest "N) word" plus two), filled column-major, as
   many columns as fit in the width, and the gap between cells made of
   tabs wherever a tab stop falls inside it.  One column when everything
   would otherwise fit on a single row.  Written to stderr in one piece. */

/* The width bash lays the menu out in: COLUMNS when it is a positive
   number, else the terminal's width, else 80 (default_columns). */
int	sel_columns(t_shell *state)
{
	char	*v;

	v = env_expand(state, "COLUMNS");
	if (v && *v && ft_atoi(v) > 0)
		return (ft_atoi(v));
	return (get_cols());
}

/* Pad from column `from` to column `to` the way bash's indent() does: a
   tab whenever the next tab stop (every 8) lies within the gap, spaces
   for the rest.  Byte-exact with bash's menu, tabs included. */
void	sel_indent(t_string *out, int from, int to)
{
	while (from < to)
	{
		if ((to / 8) > (from / 8))
		{
			vec_push_char(out, '\t');
			from += 8 - from % 8;
		}
		else
		{
			vec_push_char(out, ' ');
			from++;
		}
	}
}

/* "N) word" for item `ind` (1-based) with the index right-aligned in il
   columns; returns the word's display width, as bash's
   print_index_and_element does. */
int	sel_item(t_selmenu *m, int ind, int il)
{
	char	*w;
	char	*num;
	int		pad;

	w = ((char **)m->words->ctx)[ind - 1];
	pad = il - sel_numlen(ind);
	while (pad-- > 0)
		vec_push_char(m->out, ' ');
	num = ft_itoa(ind);
	vec_push_str(m->out, num);
	xfree(num);
	vec_push_str(m->out, ") ");
	vec_push_str(m->out, w);
	return (sel_displen(w));
}

/* One row: items row, row+rows, row+2*rows ... each padded to the cell
   width except the last; the first column's index field is as wide as
   the row count, the others as wide as the item count. */
static void	sel_row(t_selmenu *m, int row)
{
	int	ind;
	int	pos;
	int	il;
	int	len;

	ind = row;
	pos = 0;
	while (true)
	{
		il = m->idx_len;
		if (pos == 0)
			il = m->first_len;
		len = sel_item(m, ind + 1, il) + il + 2;
		ind += m->rows;
		if (ind >= (int)m->words->len)
			break ;
		sel_indent(m->out, pos + len, pos + m->max_len);
		pos += m->max_len;
	}
	vec_push_char(m->out, '\n');
}

void	select_print_menu(t_shell *state, t_vec *words)
{
	t_selmenu	m;
	t_string	out;
	int			len;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	m = (t_selmenu){.words = words, .out = &out};
	i = 0;
	while (i < (int)words->len)
	{
		len = sel_displen(((char **)words->ctx)[i++]);
		if (len > m.max_len)
			m.max_len = len;
	}
	sel_geometry(&m, sel_columns(state));
	i = 0;
	while (i < m.rows)
		sel_row(&m, i++);
	sel_write(STDERR_FILENO, (char *)out.ctx, out.len);
	xfree(out.ctx);
}
