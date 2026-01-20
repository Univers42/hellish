/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   verif.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:54:52 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 23:58:59 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

bool	is_double_open_paren(t_token t, int idx)
{
	return (idx + 1 < t.len && t.start[idx] == '(' && t.start[idx + 1] == '(');
}

bool	is_double_close_paren(t_token t, int idx)
{
	return (idx + 1 < t.len && t.start[idx] == ')' && t.start[idx + 1] == ')');
}

bool	is_open_paren(t_token t, int idx)
{
	return (t.start[idx] == '(');
}

bool	is_close_paren(t_token t, int idx)
{
	return (t.start[idx] == ')');
}

// wrappers for t_reparser
bool	is_double_open_paren_rp(t_reparser *rp)
{
	return (is_double_open_paren(rp->current_token, rp->i));
}
