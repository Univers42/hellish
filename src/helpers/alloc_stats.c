/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alloc_stats.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include <stdlib.h>

/* SAFE=0 leak oracle. ASan and valgrind are blind to ft_malloc, so with
   HELLISH_ALLOC_STATS set the main process prints the bytes still live on the
   ft_malloc heap after free_all_state(). No-op on the libc backend.

   HAVE_ALLOC_ORACLE comes from the Makefile, which already knows which heap it
   is building. This used to be decided at LINK time instead -- a weak
   undefined reference plus a -Wl,-u to drag the archive member in -- so the
   file needed no -D. That is an ELF-only trick. Mach-O reads
   __attribute__((weak)) on a DECLARATION as a weak definition rather than a
   weak import, so Apple's linker wanted a body for it and the macOS arm64
   build stopped at:

     Undefined symbols for architecture arm64:
       "_malloc_live_bytes", referenced from: _free_all_state in lto.o

   The reference below is strong, which is also what retired the -Wl,-u: a
   strong reference pulls an archive member on its own; only a weak one
   does not. */
#ifdef HAVE_ALLOC_ORACLE

size_t	malloc_live_bytes(void);

void	alloc_live_report(void)
{
	char	*n;

	if (!getenv("HELLISH_ALLOC_STATS"))
		return ;
	n = ft_itoa((int)malloc_live_bytes());
	ft_eprintf("[ft_malloc] live bytes after cleanup: %s\n", n);
	xfree(n);
}

#else

void	alloc_live_report(void)
{
}

#endif
