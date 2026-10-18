/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_dyn_time.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/26 00:10:00 by marvin            #+#    #+#             */
/*   Updated: 2026/09/26 00:10:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"
#include "ft_stdlib.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/random.h>

/* Third tier of the dynamic variables, after expand_special_dyn.

   $EPOCHREALTIME is the time since the epoch with microseconds, as bash
   prints it: seconds, a '.', six digits. It is what a prompt timer reads
   without forking `date +%s%N`; $EPOCHSECONDS has one-second resolution.

   $SRANDOM is 32 random bits per read, from the kernel (getentropy), not
   from the $RANDOM generator: bash promises that it cannot be seeded, so
   RANDOM=42 must not make it predictable. getentropy only fails on a
   kernel without the call; then the session PRNG, all 32 bits, is the
   fallback, as bash falls back to its own generator.

   Both borrow state->linebuf like $SECONDS. NULL = not one of ours. */
char	*expand_special_dyn_time(t_shell *state, char *key, int len)
{
	struct timespec	ts;
	uint32_t		r;

	if (len == 13 && ft_strncmp(key, "EPOCHREALTIME", 13) == 0)
	{
		clock_gettime(CLOCK_REALTIME, &ts);
		snprintf(state->linebuf, sizeof(state->linebuf), "%lld.%06ld",
			(long long)ts.tv_sec, ts.tv_nsec / 1000);
		return (state->linebuf);
	}
	if (len == 7 && ft_strncmp(key, "SRANDOM", 7) == 0)
	{
		if (getentropy(&r, sizeof(r)) != 0)
			r = random_uint32(&state->prng);
		snprintf(state->linebuf, sizeof(state->linebuf), "%u", r);
		return (state->linebuf);
	}
	return (NULL);
}
