/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 15:44:49 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 15:44:49 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../incs/lexer.h"

static void	advance_bs(char **str);
static char	*parse_word(t_deque *tokens, char **str);
static int	longest_matching_str(t_op_map *needles, char *haystack);
static void	parse_op(t_deque *tokens, char **str);


// If returns 0, it finished properly, if it returns a ptr, make a prompt
// with that str
char	*tokenizer(char *str, t_deque_tt *ret)
{
	char	*prompt;

	prompt = 0;
	deque_clear(ret);
	while (str && *str)
	{
		if (*str == '\'' || *str == '"' || *str == '$'
			|| !(is_special_char(*str)))
			prompt = parse_word(ret, &str);
		else if (*str == '\n')
		{
			deque_push_end(
				ret, (t_token){.start = str, .len = 1, .tt = TT_NEWLINE});
			str++;
		}
		else if (is_space(*str))
			str++;
		else
			parse_op(ret, &str);
		if (prompt)
			break ;
	}
	deque_push_end(ret, (t_token){.tt = TT_END});
	return (prompt);
}

static void	advance_bs(char **str)
{
	ft_assert(**str == '\\');
	if ((*str)[1])
		*str += 1;
	*str += 1;
}

static char	*parse_word(t_deque_tt *tokens, char **str)
{
	char	*start;

	start = *str;
	while (**str)
	{
		if (**str == '\\')
			advance_bs(str);
		else if (!is_special_char(**str) || **str == '$')
			(*str)++;
		else if (**str == '\'')
		{
			if (advance_squoted(str))
				return (tokens->looking_for = '\'', LEXER_SQUOTE_PROMPT);
		}
		else if (**str == '"')
		{
			if (advance_dquoted(str))
				return (tokens->looking_for = '"', LEXER_DQUOTE_PROMPT);
		}
		else
			break ;
	}
	deque_push_end(
		tokens, (t_token){.start = start, .len = *str - start, .tt = TT_WORD});
	return (0);
}

static int	longest_matching_str(t_op_map *needles, char *haystack)
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

static void	parse_op(t_deque_tt *tokens, char **str)
{
	char		*start;
	int			op_idx;
	t_op_map	operators[13];

	operators[0] = (t_op_map){"|", TT_PIPE};
	operators[1] = (t_op_map){"<<", TT_HEREDOC};
	operators[2] = (t_op_map){"<<-", TT_HEREDOC};
	operators[3] = (t_op_map){">>", TT_APPEND};
	operators[4] = (t_op_map){"(", TT_BRACE_LEFT};
	operators[5] = (t_op_map){")", TT_BRACE_RIGHT};
	operators[6] = (t_op_map){">", TT_REDIRECT_RIGHT};
	operators[7] = (t_op_map){"<", TT_REDIRECT_LEFT};
	operators[8] = (t_op_map){"&&", TT_AND};
	operators[9] = (t_op_map){"&", TT_SEMICOLON};
	operators[10] = (t_op_map){"||", TT_OR};
	operators[11] = (t_op_map){";", TT_SEMICOLON};
	operators[12] = (t_op_map){0, TT_END};
	start = *str;
	op_idx = longest_matching_str(operators, *str);
	ft_assert(op_idx != -1);
	*str += ft_strlen(operators[op_idx].str);
	deque_push_end(tokens, (t_token){.start = start,
		.len = *str - start,
		.tt = operators[op_idx].t});
}
