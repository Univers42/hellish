/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alloc_stats.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include <stdlib.h>

/* ft_malloc's leak oracle, imported weakly so this TU is backend-agnostic and
   needs no -D: at SAFE=0 it binds to the real symbol in libft; at SAFE=1 (libc)
   the symbol is absent and resolves to NULL, so the report is skipped. */
size_t	malloc_live_bytes(void) __attribute__((weak));

/* SAFE=0 leak oracle. ASan/valgrind are blind to ft_malloc, so with
   HELLISH_ALLOC_STATS set the main process prints the bytes still live on the
   ft_malloc heap after free_all_state(). No-op on the libc backend. */
void	alloc_live_report(void)
{
	char	*n;

	if (!malloc_live_bytes || !getenv("HELLISH_ALLOC_STATS"))
		return ;
	n = ft_itoa((int)malloc_live_bytes());
	ft_eprintf("[ft_malloc] live bytes after cleanup: %s\n", n);
	xfree(n);
}
