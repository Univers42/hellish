/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   quote_pname.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 14:06:12 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 14:14:44 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"

//Remove backslages quoting character in Pathname by modifying pathname
void    udequote_pathname(char *path_name)
{
	int	i;
	int	j;

	i = 0;
	j = 0;
	while (path_name && path_name[i])
	{
		if (path_name[i] == '\\')
			i++;
		path_name[j++] = path_name[i++];
		if (path_name[i - 1] == 0)
			break ;
	}
	if (path_name)
		path_name[j] = '\0';
}

