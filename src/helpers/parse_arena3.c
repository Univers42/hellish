/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_arena3.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/20 19:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/07/20 19:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parena.h"
#include "ft_memory.h"

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

/* The shared rounding step of the bump allocator: parena_alloc and
   parena_try_extend must agree byte-for-byte on rounded sizes or the
   arena-tip test misfires and extends the wrong block. */
size_t	parena_round(size_t n)
{
	return ((n + 7) & ~(size_t)7);
}
