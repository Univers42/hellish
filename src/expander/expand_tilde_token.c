/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_tilde_token.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:31:00 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:31:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "parena.h"
#include "sys.h"
#include <pwd.h>
#include <unistd.h>

/* The home directory for a bare ~ (or ~/): $HOME when set, else the password
   database entry for the real uid -- bash's fallback when HOME is unset. NULL
   only if even that lookup fails, leaving the ~ literal (POSIX). */
static char	*tilde_home_dir(t_shell *state)
{
	char			*home;
	struct passwd	*pw;

	home = env_expand(state, HOME);
	if (home)
		return (home);
	pw = getpwuid(getuid());
	if (pw)
		return (pw->pw_dir);
	return (NULL);
}

/* ~name[/...] : splice in `name`'s home dir (getpwnam). Unknown user -> the
   word is left unchanged (POSIX leaves an unknown ~login literal). */
static void	expand_tilde_user(t_token *t)
{
	int				i;
	char			*user;
	struct passwd	*pw;
	t_string		s;

	i = 1;
	while (i < t->len && t->start[i] != '/')
		i++;
	user = ft_substr(t->start, 1, (size_t)(i - 1));
	if (!user)
		return ;
	pw = getpwnam(user);
	xfree(user);
	if (!pw)
		return ;
	vec_init(&s);
	vec_push_str(&s, pw->pw_dir);
	vec_push_nstr(&s, t->start + i, t->len - i);
	t->start = (char *)s.ctx;
	t->len = s.len;
	t->allocated = true;
	parena_note_attach();
}

/* Splice `val` over the first `plen` bytes of token `t`, keeping the rest
   of the word.  The joined buffer is a fresh allocation the token now owns
   (allocated flag) and the parena is told about the attachment. */
static void	tilde_splice(t_token *t, char *val, int plen)
{
	t_string	s;

	vec_init(&s);
	vec_push_str(&s, val);
	t->allocated = true;
	parena_note_attach();
	vec_push_nstr(&s, t->start + plen, t->len - plen);
	t->start = (char *)s.ctx;
	t->len = s.len;
}

/* Replace the leading tilde prefix of token `t` with the appropriate path.
   Dispatch: ~+ → $PWD, ~- → $OLDPWD, ~name → getpwnam(name)->pw_dir,
   ~ alone → $HOME (or the passwd home when HOME is unset, see tilde_home_dir).
   If the required value is absent the token is left unchanged (POSIX:
   unresolvable tilde stays literal). */
void	expand_tilde_token(t_shell *state, t_token *t)
{
	int		template_len;
	char	*env_val;

	template_len = 2;
	if (token_starts_with(*t, CUR_DIR))
		env_val = env_expand(state, PWD);
	else if (token_starts_with(*t, BEFORE))
		env_val = env_expand(state, OLDPWD);
	else if (t->len >= 2 && t->start[1] != '/')
		return (expand_tilde_user(t));
	else
	{
		env_val = tilde_home_dir(state);
		template_len = 1;
	}
	if (!env_val)
		return ;
	tilde_splice(t, env_val, template_len);
}
