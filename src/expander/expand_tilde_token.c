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
#include "sys.h"
#include <pwd.h>

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
	free(user);
	if (!pw)
		return ;
	vec_init(&s);
	vec_push_str(&s, pw->pw_dir);
	vec_push_nstr(&s, t->start + i, t->len - i);
	t->start = (char *)s.ctx;
	t->len = s.len;
	t->allocated = true;
}

void	expand_tilde_token(t_shell *state, t_token *t)
{
	int			template_len;
	char		*env_val;
	t_string	s;

	template_len = 2;
	if (token_starts_with(*t, CUR_DIR))
		env_val = env_expand(state, PWD);
	else if (token_starts_with(*t, BEFORE))
		env_val = env_expand(state, OLDPWD);
	else if (t->len >= 2 && t->start[1] != '/')
		return (expand_tilde_user(t));
	else
	{
		env_val = env_expand(state, HOME);
		template_len = 1;
	}
	if (!env_val)
		return ;
	vec_init(&s);
	if (env_val)
		vec_push_str(&s, env_val);
	t->allocated = true;
	vec_push_nstr(&s, t->start + template_len, t->len - template_len);
	t->start = (char *)s.ctx;
	t->len = s.len;
}
