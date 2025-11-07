/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 00:41:04 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 00:48:14 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"

typedef struct s_list
{
	struct s_list *next;
	void			*data;
}t_list;
void    add_glob(const t_glob *glob)
{
	char    **p;

	p = pglob->gl_pathv;
	while (*++p)
		add_fname(*p);
}

char    *pre_glob(const char *pattern, int flag)
{
	if (FNM)
}

void    expandmeta_glob(t_list *str)
{
		
}

void	expand_meta(t_list *str)
{

}

char	*expmeta(char *, )
{

}

int		patmatch()
{

}

int		pmatch()
{

}
