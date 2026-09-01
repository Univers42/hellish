/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parena.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/20 19:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/07/20 19:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Parse arena: a chunked bump allocator for cycle-lifetime parse objects
   (AST children buffers, full_word back-refs, escape-processed copies).
   One input cycle allocates thousands of tiny blocks that all die together
   at cycle end; bump allocation + one reset beats per-block malloc/free.
   The arena is GATED: parena_on(true) only around the main input cycle's
   parse_tokens call, so nested parses (eval, source, command substitution
   bodies) fall through to xmalloc and keep their own exact free discipline
   — an `eval` inside a `while true` loop must not grow the arena forever.
   parena_free() routes: arena pointers are a no-op (reclaimed wholesale by
   parena_reset), anything else is forwarded to xfree — so heap-built trees
   (function-body clones, eval ASTs) free through the same call sites. */

#ifndef PARENA_H
# define PARENA_H

# include <stddef.h>
# include <stdbool.h>

/* Overridable so a stress build can shrink the chunks (e.g.
   -DPARENA_FIRST_CHUNK=512) and turn rare chunk-boundary states into
   the common case — tests/alloc_stress.sh builds exactly that. */
# ifndef PARENA_MAX_CHUNKS
#  define PARENA_MAX_CHUNKS 64
# endif
# ifndef PARENA_FIRST_CHUNK
#  define PARENA_FIRST_CHUNK 262144
# endif
# ifndef PARENA_MAX_CHUNK
#  define PARENA_MAX_CHUNK 8388608
# endif

/* Bump-allocation granularity is 8 bytes: every parse object is
   pointer-aligned, and small blocks waste half as much padding as the
   old 16. alloc and try_extend MUST round through this one function or
   the tip test in try_extend misfires (it replaced a macro the 42 norm
   forbids; LTO inlines it right back on optimized builds). */
size_t		parena_round(size_t n);

typedef struct s_parena
{
	void	*chunk[PARENA_MAX_CHUNKS];
	size_t	size[PARENA_MAX_CHUNKS];
	int		n_chunks;
	int		cur; /* chunk currently bump-allocated from */
	size_t	off; /* bump offset inside chunk[cur] */
	bool	on; /* gate: false -> parena_alloc falls through to xmalloc */
	bool	attached; /* heap memory attached to the cycle tree this cycle */
}	t_parena;

t_parena	*parena(void);
void		*parena_alloc(size_t n);
bool		parena_owns(const void *p);
void		parena_free(void *p);
void		parena_on(bool on);
void		parena_reset(void);
void		parena_destroy(void);
bool		parena_try_extend(void *p, size_t old_n, size_t new_n);
void		parena_note_attach(void);

#endif
