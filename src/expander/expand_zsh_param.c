/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_param.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 01:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 01:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "ft_builtins.h"
#include "cmd_hash.h"
#include "parena.h"

char	*exe_path(char **path_dirs, char *exe_name, int *perm_denied);
void	path_cache_sync(t_shell *state);

/* zsh's `${+name}` and the special associative arrays behind it.
**
**     (( $+commands[pixz] )) && tar -I pixz -xvf "$f"
**
** is how every zsh plugin asks "is this program installed?", and it is the
** only construct left between oh-my-zsh's extract plugin and loading. The
** corpus uses `$+commands[x]` twelve times and `${commands[x]}` once;
** nothing else, so nothing else is built here.
**
** `${+name}` is 1 when the parameter is SET and 0 when it is not -- note
** that a set-but-empty variable is 1, which is the whole reason the form
** exists rather than testing the value.
**
** The special arrays are computed on demand rather than materialised: zsh
** builds `commands` from a hash of $PATH, and keeping one in sync would
** mean invalidating it on every PATH change, every new executable, and
** every `hash -r`. A lookup answers the one question actually asked.
*/

/* The subscript of `name[key]`, or NULL when there is none.  Sets *nlen to
   the name length so the caller can compare the base name. */
static const char	*zp_subscript(const char *s, int slen, int *nlen,
						int *klen)
{
	int	i;

	i = 0;
	while (i < slen && s[i] != '[')
		i++;
	if (i >= slen || s[slen - 1] != ']')
		return (NULL);
	*nlen = i;
	*klen = slen - i - 2;
	if (*klen < 0)
		return (NULL);
	return (s + i + 1);
}

/* Is `key` a live thing of the kind `base` names?  The three special tables
   zsh exposes, then ordinary parameters. */
static int	zp_exists(t_shell *state, const char *base, int blen, char *key)
{
	char	*path;

	if (blen == 8 && !ft_strncmp(base, "commands", 8))
	{
		path = zp_which(state, key);
		if (path)
			return (xfree(path), 1);
		return (0);
	}
	if (blen == 9 && !ft_strncmp(base, "functions", 9))
		return (func_lookup(state, key) != NULL);
	if (blen == 7 && !ft_strncmp(base, "aliases", 7))
		return (hash_get(&state->aliases, key) != NULL);
	return (zp_elem_set(state, base, blen, key));
}

/* ${+name} and ${+name[key]}: 1 if set, 0 if not.  Returns NULL when the
   body is not a `+` form at all. */
static char	*zp_plus(t_shell *state, const char *s, int slen)
{
	const char	*sub;
	char		*key;
	int			nlen;
	int			klen;
	int			r;

	sub = zp_subscript(s + 1, slen - 1, &nlen, &klen);
	if (!sub)
		return (ft_itoa(env_expand_n(state, (char *)s + 1, slen - 1) != NULL));
	key = ft_strndup(sub, (size_t)klen);
	r = zp_exists(state, s + 1, nlen, key);
	xfree(key);
	return (ft_itoa(r));
}

/* Token-level entry, tried before the ARRAY forms: `$commands[ls]` looks
   exactly like an ordinary array subscript, so expand_array_token would
   claim it first and answer with the empty element of an array nobody
   defined -- a plausible "not installed" for a program that is. */
bool	zsh_param_token(t_shell *state, t_token *tt)
{
	char	*v;

	v = zsh_param(state, tt->start, tt->len);
	if (!v)
		return (false);
	tt->start = v;
	tt->len = (int)ft_strlen(v);
	tt->allocated = true;
	parena_note_attach();
	return (true);
}

/* Entry point, tried before the bash forms and only in zsh mode.  Returns a
   fresh string, or NULL to fall through unchanged. */
char	*zsh_param(t_shell *state, const char *s, int slen)
{
	const char	*sub;
	char		*key;
	char		*out;
	int			nlen;
	int			klen;

	if (!zsh_mode(state) || slen < 2)
		return (NULL);
	if (s[0] == '+')
		return (zp_plus(state, s, slen));
	sub = zp_subscript(s, slen, &nlen, &klen);
	if (!sub || nlen != 8 || ft_strncmp(s, "commands", 8))
		return (NULL);
	key = ft_strndup(sub, (size_t)klen);
	out = zp_which(state, key);
	xfree(key);
	if (!out)
		return (ft_strdup(""));
	return (out);
}
