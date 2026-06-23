/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_select.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AI_SELECT_H
# define AI_SELECT_H

# include "ai.h"
# include <termios.h>

/* State for the fuzzy picker: the full name list, the matched-index subset,
   the cursor, the lines drawn last frame (for in-place redraw) and the live
   filter buffer. */
typedef struct s_aisel
{
	char	**all;
	int		n;
	int		*idx;
	int		m;
	int		sel;
	int		prev;
	int		flen;
	char	filt[128];
}	t_aisel;

int		ai_raw_on(struct termios *saved);
void	ai_refilter(t_aisel *s);
int		ai_on_key(t_aisel *s, unsigned char *b, int r);

#endif
