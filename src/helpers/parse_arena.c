/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_arena.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/20 19:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/07/20 19:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parena.h"
#include "ft_memory.h"
#include "libft.h"

/* Process-wide arena, like the word slab: a singleton because the reparse
   pass allocates deep inside call chains that do not carry t_shell. Same
   cleanup-target caveat as the other legacy statics. Chunks are xmalloc'd
   so the compile-time heap selection (libc vs ft_malloc) is preserved and
   no pointer ever crosses heaps. */
static t_parena	g_parena;

t_parena	*parena(void)
{
	return (&g_parena);
}

/* Keep the chunk registry sorted by base address so parena_owns can
   binary-search. Chunks are created rarely (a dozen per giant cycle) but
   owns() runs on every routed free of the teardown walk — millions of
   times for a 50k-line script. */
static void	insert_sorted(void *chunk, size_t sz)
{
	int	i;

	i = g_parena.n_chunks;
	while (i > 0 && g_parena.chunk[i - 1] > chunk)
	{
		g_parena.chunk[i] = g_parena.chunk[i - 1];
		g_parena.size[i] = g_parena.size[i - 1];
		if (g_parena.cur == i - 1)
			g_parena.cur = i;
		i--;
	}
	g_parena.chunk[i] = chunk;
	g_parena.size[i] = sz;
	g_parena.cur = i;
	g_parena.n_chunks++;
}

/* Open a fresh chunk big enough for `need`. Chunk sizes double from
   PARENA_FIRST_CHUNK up to PARENA_MAX_CHUNK so small cycles stay small and
   a 50k-line script reaches its ~50MB of AST in a dozen chunks. Returns
   false when the registry is full — the caller then falls back to plain
   xmalloc, which parena_free routes correctly (owns() says no). */
static bool	parena_grow(size_t need)
{
	size_t	sz;
	void	*chunk;
	int		i;

	if (g_parena.n_chunks >= PARENA_MAX_CHUNKS)
		return (false);
	sz = PARENA_FIRST_CHUNK;
	i = 0;
	while (i < g_parena.n_chunks && sz < (size_t)PARENA_MAX_CHUNK)
	{
		sz *= 2;
		i++;
	}
	if (sz < need)
		sz = need;
	chunk = xmalloc(sz);
	if (!chunk)
		return (false);
	insert_sorted(chunk, sz);
	g_parena.off = 0;
	return (true);
}

void	*parena_alloc(size_t n)
{
	void	*p;

	n = PARENA_ROUND(n);
	if (!g_parena.on)
		return (xmalloc(n));
	if (g_parena.n_chunks == 0
		|| g_parena.off + n > g_parena.size[g_parena.cur])
	{
		if (!parena_grow(n))
			return (g_parena.attached = true, xmalloc(n));
	}
	p = (char *)g_parena.chunk[g_parena.cur] + g_parena.off;
	g_parena.off += n;
	return (p);
}

/* Binary search over the address-sorted chunk registry: find the last
   chunk whose base is <= p, then bounds-check against its size. owns()
   runs per routed free on the teardown walk (millions of calls for a big
   script), so the log2(n_chunks) probe matters. */
bool	parena_owns(const void *p)
{
	int			lo;
	int			hi;
	int			mid;
	const char	*c;

	lo = 0;
	hi = g_parena.n_chunks - 1;
	while (lo < hi)
	{
		mid = (lo + hi + 1) / 2;
		if ((const char *)g_parena.chunk[mid] <= (const char *)p)
			lo = mid;
		else
			hi = mid - 1;
	}
	if (g_parena.n_chunks == 0)
		return (false);
	c = (const char *)g_parena.chunk[lo];
	return ((const char *)p >= c && (const char *)p < c + g_parena.size[lo]);
}

/* Free router: arena blocks are reclaimed wholesale by parena_reset, so
   they are a no-op here; anything else goes to the real heap free. This
   lets one call site tear down both arena-backed cycle trees and
   heap-backed clones (function bodies, eval ASTs). */
void	parena_free(void *p)
{
	if (!p || parena_owns(p))
		return ;
	xfree(p);
}
