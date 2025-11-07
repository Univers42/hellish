/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   skip_name.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 14:58:17 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 14:59:37 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"

/*handle multibyte

*/
static int	wskipname(wchar_t *pat, wchar_t *dname, int flags)
{
	int	skip;

	skip = 0;
	if (get_global()->glob_always_skip_dot_and_dotdot
		&& WDOT_OR_DOTDOT(dname))
		return (1);
	if (get_global()->noglob_dot_filenames == 0
		&& pat[0] != L'.'
		&& (pat[0] != L'\\' || pat[1] != L'.')
		&& WDOT_OR_DOTDOT(dname))
		return (1);
	if (get_global()->noglob_dot_filenames
		&& dname[0] == L'.'
		&& pat[0] != L'.'
		&& (pat[0] != L'\\' || pat[1] != L'.'))
		skip = 1;
	(void)flags;
	return (skip);
}
