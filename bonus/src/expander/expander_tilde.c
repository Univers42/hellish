/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_tilde.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:33 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:33 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft/libft.h"
#include "../shell.h"
#include <stdbool.h>

bool	token_starts_with(t_token t, char *str)
{
	if (t.len < (int)ft_strlen(str))
		return (false);
	return (ft_strncmp(t.start, str, ft_strlen(str)) == 0);
}

void	expand_tilde_token(t_state *state, t_token *t)
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

void	expand_tilde_word(t_state *state, t_ast_node *curr)
{
	t_token	*first;
	bool	should_expand;

	ft_assert(curr->children.len);
	if (((t_ast_node *)curr->children.ctx)[0].token.tt != TT_WORD)
		return ;
	first = &((t_ast_node *)curr->children.ctx)[0].token;
	should_expand = false;
	should_expand |= token_starts_with(*first, "~")
		&& curr->children.len == 1 && first->len == 1;
	should_expand |= token_starts_with(*first, "~/");
	should_expand |= token_starts_with(*first, "~+")
		&& curr->children.len == 1 && first->len == 2;
	should_expand |= token_starts_with(*first, "~+/");
	should_expand |= token_starts_with(*first, "~-")
		&& curr->children.len == 1 && first->len == 2;
	should_expand |= token_starts_with(*first, "~-/");
	if (should_expand)
		expand_tilde_token(state, first);
}
