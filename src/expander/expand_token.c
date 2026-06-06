/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_token.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:30:31 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:30:31 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

char	*expand_param_format(t_shell *state, const char *s, int slen);
void	nounset_abort(t_shell *state, const char *name, int len);

/* Join the positional parameters $1..$# with the first char of IFS (space by
   default). Used for $@/$*; quoted "$@" multi-field is a known limitation. */
char	*join_positionals(t_shell *state)
{
	t_string	out;
	char		*cnt;
	char		*k;
	char		*v;
	int			i;

	cnt = env_expand(state, "#");
	i = 0;
	vec_init(&out);
	out.elem_size = 1;
	while (cnt && ++i <= ft_atoi(cnt))
	{
		k = ft_itoa(i);
		v = env_expand(state, k);
		xfree(k);
		if (i > 1 && env_get_ifs(&state->env))
			vec_push(&out, &env_get_ifs(&state->env)[0]);
		if (v)
			vec_push_str(&out, v);
	}
	return (vec_push(&out, &(char){0}), (char *)out.ctx);
}

static bool	handle_empty_token(t_token *curr_tt)
{
	if (curr_tt->len != 0)
		return (false);
	if (curr_tt->start && (*curr_tt->start == '\''
			|| *curr_tt->start == '"'))
	{
		curr_tt->start = "";
		curr_tt->len = 0;
		curr_tt->allocated = false;
		return (true);
	}
	curr_tt->start = "$";
	curr_tt->len = 1;
	curr_tt->allocated = false;
	return (true);
}

static void	expand_simple_var(t_shell *state, t_token *curr_tt)
{
	char	*temp;

	temp = env_expand_n(state, curr_tt->start, curr_tt->len);
	if (!temp && state->opt_nounset)
		nounset_abort(state, curr_tt->start, curr_tt->len);
	curr_tt->start = temp;
	if (curr_tt->start)
		curr_tt->len = ft_strlen(temp);
	else
		curr_tt->len = 0;
	curr_tt->allocated = false;
}

static bool	expand_positional(t_shell *state, t_token *curr_tt, bool split_ctx)
{
	char	*temp;

	if (split_ctx && curr_tt->tt == TT_DQENVVAR && curr_tt->len == 1
		&& curr_tt->start[0] == '@')
		return (true);
	if (curr_tt->len != 1)
		return (false);
	if (curr_tt->start[0] != '@' && curr_tt->start[0] != '*')
		return (false);
	temp = join_positionals(state);
	curr_tt->start = temp;
	curr_tt->len = (int)ft_strlen(temp);
	curr_tt->allocated = true;
	return (true);
}

void	expand_token(t_shell *state, t_token *curr_tt, bool split_ctx)
{
	char	*fmt;

	if (expand_positional(state, curr_tt, split_ctx))
		return ;
	if (handle_empty_token(curr_tt))
		return ;
	fmt = expand_param_format(state, curr_tt->start, curr_tt->len);
	if (fmt)
	{
		curr_tt->start = fmt;
		curr_tt->len = ft_strlen(fmt);
		curr_tt->allocated = true;
		return ;
	}
	expand_simple_var(state, curr_tt);
}
