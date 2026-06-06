/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   x_getcwd.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sys.h"
#include "libft.h"
#include <unistd.h>
#include <stdlib.h>

/* getcwd(NULL, 0) hands back a libc-malloc'd buffer; copy it onto the active
   heap (ft_strdup -> fn_malloc) and libc-free the original, so every cwd
   pointer the shell stores or xfree()s lives on the same heap as the rest of
   its memory. Without this, a getcwd buffer escaping into a struct (e.g. the
   dir stack) and later xfree()d would corrupt ft_malloc at SAFE=0. Returns
   NULL when the cwd is unavailable. */
char	*x_getcwd(void)
{
	char	*raw;
	char	*dup;

	raw = getcwd(NULL, 0);
	if (!raw)
		return (NULL);
	dup = ft_strdup(raw);
	free(raw);
	return (dup);
}
