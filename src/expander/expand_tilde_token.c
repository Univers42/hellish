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

void	expand_tilde_token(t_shell *state, t_token *t)
{
	int			template_len;
	char		*env_val;
	t_string	s;

	template_len = 2;
	if (token_starts_with(*t, "~+"))
		env_val = env_expand(state, "PWD");
	else if (token_starts_with(*t, "~-"))
		env_val = env_expand(state, "OLDPWD");
	else
	{
		env_val = env_expand(state, "HOME");
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