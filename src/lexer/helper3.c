/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper3.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:00:38 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 23:00:38 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

int	longest_matching_str(t_op_map *needles, char *haystack)
{
	int	max_idx;
	int	max;
	int	i;

	max_idx = -1;
	max = -1;
	i = 0;
	while (needles[i].str)
	{
		if ((int)ft_strlen(needles[i].str) > max
			&& ft_strncmp(needles[i].str, haystack,
				ft_strlen(needles[i].str)) == 0)
		{
			max_idx = i;
			max = ft_strlen(needles[i].str);
		}
		i++;
	}
	return (max_idx);
}

/* fill pipe and heredoc/append family */
static void	init_ops_group1(t_op_map ops[])
{
	ops[0] = (t_op_map){"|", TT_PIPE};
	ops[1] = (t_op_map){"<<", TT_HEREDOC};
	ops[2] = (t_op_map){"<<-", TT_HEREDOC};
	ops[3] = (t_op_map){"<", TT_REDIRECT_LEFT};
	ops[4] = (t_op_map){"(", TT_BRACE_LEFT};
	ops[5] = (t_op_map){"((", TT_ARITH_START};
}

/* fill arithmetic/braces and process-substitution family */
static void	init_ops_group2(t_op_map ops[])
{
	ops[6] = (t_op_map){")", TT_BRACE_RIGHT};
	ops[7] = (t_op_map){"<(", TT_PROC_SUB_IN};
	ops[8] = (t_op_map){">(", TT_PROC_SUB_OUT};
	ops[9] = (t_op_map){">>", TT_APPEND};
}

/* fill redirects, logicals and misc */
static void	init_ops_group3(t_op_map ops[])
{
	ops[10] = (t_op_map){">", TT_REDIRECT_RIGHT};
	ops[11] = (t_op_map){"&&", TT_AND};
	ops[12] = (t_op_map){"&", TT_AMPERSAND};
	ops[13] = (t_op_map){"||", TT_OR};
	ops[14] = (t_op_map){";", TT_SEMICOLON};
	ops[15] = (t_op_map){0, TT_END};
}

void	parse_op(t_deque_tt *tokens, char **str)
{
	char		*start;
	int			op_idx;
	t_op_map	operators[17];
	t_token		tmp;

	init_ops_group1(operators);
	init_ops_group2(operators);
	init_ops_group3(operators);
	start = *str;
	op_idx = longest_matching_str(operators, *str);
	ft_assert(op_idx != -1);
	*str += ft_strlen(operators[op_idx].str);
	tmp = create_token(start, (int)(*str - start), operators[op_idx].t);
	deque_push_end(&tokens->deqtok, &tmp);
}
