/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   finddirs.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 14:15:18 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 14:18:56 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"

t_list  *find_dirs(char *p, char *sdir, int flags, t_list **ep, int *np)
{
	char    **r;
	int		ndirs;
	t_list	*ret;
	t_list	*e;
	t_list *g;

	e = 0;
	ret = 0;
	r = glob_vector(p, sdir, flags);
	if (r == 0 || r[0] == 0)
	{
		if (np)
			*np = 0;
		if (ep)
			*ep = 0;
		if (r && r != &glob_er)
	}
}