/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   word_slab.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "slab.h"
#include "ft_string.h"
#include "ft_memory.h"
#include <stdlib.h>

/* Process-wide slab for short-lived simple-command argv word strings: most are
   tiny and die together at command end, so size-class freelists beat the
   general heap. word_free() routes any non-slab pointer to slab_free(), which
   forwards it to fn_free() (the active backend) — so a command's argv (a mix
   of slab literals + xmalloc'd expansions) frees safely through it. The g_on
   flag is force-set per expansion context (argv on, assignment/for off) so
   values escaping into env stay on the fn_* heap; since word_free handles both,
   a wrong flag can only cost speed, never corrupt. */
static t_slab_allocator	g_wslab;
static int				g_wslab_up;
static int				g_on;

/* Enable or disable slab allocation for the current expansion context,
   returning the previous state so callers can restore it. Pass 1 for argv
   words (they die together at command end), 0 for assignment/for-loop values
   that may survive into the environment and must live on the general heap. */
int	word_slab_push(int on)
{
	int	old;

	old = g_on;
	g_on = on;
	return (old);
}

/* Lazy-init the slab on first use: five size classes cover the common word
   lengths (16/32/64/128/256 bytes) with generous slab counts (512/512/
   256/128/64 objects per cache). Larger strings fall through to xmalloc in
   word_strndup but that is rare -- shell argv words are nearly always short. */
static void	word_slab_boot(void)
{
	slab_init(&g_wslab);
	slab_add_cache(&g_wslab, 16, 512);
	slab_add_cache(&g_wslab, 32, 512);
	slab_add_cache(&g_wslab, 64, 256);
	slab_add_cache(&g_wslab, 128, 128);
	slab_add_cache(&g_wslab, 256, 64);
	g_wslab_up = 1;
}

/* Duplicate n bytes of s into a NUL-terminated string. Picks the slab when
   g_on is set (argv context) or falls back to xmalloc when slab is off or
   the allocation is larger than the biggest cache. The slab is booted on
   first use so no explicit initialisation call is required at startup. */
char	*word_strndup(const char *s, size_t n)
{
	char	*p;

	if (!g_on)
	{
		p = xmalloc(n + 1);
		if (!p)
			return (NULL);
		return (ft_memcpy(p, s, n), p[n] = '\0', p);
	}
	if (!g_wslab_up)
		word_slab_boot();
	p = (char *)slab_alloc(&g_wslab, n + 1);
	if (!p)
		return (NULL);
	return (ft_memcpy(p, s, n), p[n] = '\0', p);
}

/* Release a word pointer regardless of which allocator produced it. When the
   slab is up, slab_free() routes to the right cache or to fn_free() for
   oversized/xmalloc pointers. When the slab was never started (g_wslab_up==0)
   we fall straight through to xfree -- this handles early-exit paths where
   word_slab_boot() was never called. NULL is always safe. */
void	word_free(void *p)
{
	if (p == NULL)
		return ;
	if (!g_wslab_up)
		return ((void)xfree(p));
	slab_free(&g_wslab, p);
}

/* Release the process-wide slab at shutdown (main process only). The chunks are
   a lifetime cache reachable via g_wslab, so they are not a logical leak, but
   tearing them down keeps the ft_malloc live-bytes oracle at zero on exit. */
void	word_slab_teardown(void)
{
	if (g_wslab_up)
		slab_destroy(&g_wslab);
	g_wslab_up = 0;
}
