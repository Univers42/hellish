/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   brand.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/19 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/11/19 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "brand.h"
#include "libft.h"
#include <stdlib.h>
#include <unistd.h>

/* PREFIX + suffix into buf. The knob names are compile-time literals well
   under 64 bytes; a longer one is refused rather than truncated, because a
   truncated name would silently read some other variable. */
static int	brand_key(char *buf, const char *prefix, const char *suffix)
{
	if (ft_strlen(prefix) + ft_strlen(suffix) >= 64)
		return (0);
	ft_strlcpy(buf, prefix, 64);
	ft_strlcat(buf, suffix, 64);
	return (1);
}

/**
 * @brief Choose between a new-name path and its pre-3.4.0 equivalent.
 *
 * The new path wins when it exists, or when the legacy one does not (a fresh
 * install). Only an upgraded machine that has the legacy path and not the new
 * one keeps reading the legacy path -- so creating ~/.polishrc is how a user
 * moves over, and nothing ever has to be migrated behind their back.
 *
 * @param fresh  heap path under the new name (owned; may be NULL)
 * @param legacy heap path under the old name (owned; may be NULL)
 * @return the chosen path (caller frees); the other one is freed here
 */
char	*brand_pick(char *fresh, char *legacy)
{
	if (fresh && (access(fresh, F_OK) == 0
			|| !legacy || access(legacy, F_OK) != 0))
		return (xfree(legacy), fresh);
	xfree(fresh);
	return (legacy);
}

/* Process-environment knob (read before or outside the shell's own variable
   table): POLISH_<suffix> if set at all, else HELLISH_<suffix>. Set-ness,
   not truthiness, decides -- POLISH_BANNER=0 must override HELLISH_BANNER=1. */
const char	*brand_getenv(const char *suffix)
{
	char		key[64];
	const char	*v;

	if (!brand_key(key, BRAND_PREFIX, suffix))
		return (NULL);
	v = getenv(key);
	if (v)
		return (v);
	if (!brand_key(key, BRAND_LEGACY_PREFIX, suffix))
		return (NULL);
	return (getenv(key));
}

/* Shell-variable knob, same precedence as brand_getenv. */
t_env	*brand_env(t_shell *state, const char *suffix)
{
	char	key[64];
	t_env	*e;

	if (!brand_key(key, BRAND_PREFIX, suffix))
		return (NULL);
	e = env_get(&state->env, key);
	if (e)
		return (e);
	if (!brand_key(key, BRAND_LEGACY_PREFIX, suffix))
		return (NULL);
	return (env_get(&state->env, key));
}

/* Borrowed value (env_expand's contract: never free; NULL means unset). */
char	*brand_expand(t_shell *state, const char *suffix)
{
	char	key[64];
	char	*v;

	if (!brand_key(key, BRAND_PREFIX, suffix))
		return (NULL);
	v = env_expand(state, key);
	if (v)
		return (v);
	if (!brand_key(key, BRAND_LEGACY_PREFIX, suffix))
		return (NULL);
	return (env_expand(state, key));
}
