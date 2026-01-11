/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:59 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:59 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

/* new: operator map type used by this lexer */

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
	t_token	tmp;

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
	tmp = (t_token){.start = start, .len = (int)(*str - start), .tt = TT_WORD};
	deque_push_end(&tokens->deqtok, &tmp);
	return (0);
}

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

void	parse_op(t_deque_tt *tokens, char **str)
{
	char		*start;
	int			op_idx;
	t_op_map	operators[13];
	t_token		tmp;

	operators[0] = (t_op_map){"|", TT_PIPE};
	operators[1] = (t_op_map){"<<", TT_HEREDOC};
	operators[2] = (t_op_map){"<<-", TT_HEREDOC};
	/* correct append operator */
	operators[3] = (t_op_map){">>", TT_APPEND};
	operators[4] = (t_op_map){"(", TT_BRACE_LEFT};
	operators[5] = (t_op_map){")", TT_BRACE_RIGHT};
	operators[6] = (t_op_map){">", TT_REDIRECT_RIGHT};
	operators[7] = (t_op_map){"<", TT_REDIRECT_LEFT};
	operators[8] = (t_op_map){"&&", TT_AND};
	/* treat single '&' as a simple list separator (like ';') so
	   inputs without spacing (e.g. "<file&cmd") are parsed */
	operators[9] = (t_op_map){"&", TT_SEMICOLON};
	operators[10] = (t_op_map){"||", TT_OR};
	operators[11] = (t_op_map){";", TT_SEMICOLON};
	operators[12] = (t_op_map){0, TT_END};
	start = *str;
	op_idx = longest_matching_str(operators, *str);
	ft_assert(op_idx != -1);
	*str += ft_strlen(operators[op_idx].str);
	tmp = (t_token){.start = start,
		.len = (int)(*str - start),
		.tt = operators[op_idx].t};
	deque_push_end(&tokens->deqtok, &tmp);
}

// If returns 0, it finished properly, if it returns a ptr, make a prompt
// with that str
char	*tokenizer(char *str, t_deque_tt *ret)
{
	char	*prompt;

	prompt = 0;
	deque_clear(&ret->deqtok, NULL);
	while (str && *str)
	{
		if (*str == '\'' || *str == '"' || *str == '$'
			|| !(is_special_char(*str)))
			prompt = parse_word(ret, &str);
		else if (*str == '\n')
		{
			t_token tmp = (t_token){.start = str, .len = 1, .tt = TT_NEWLINE};
			deque_push_end(&ret->deqtok, &tmp);
			str++;
		}
		else if (is_space(*str))
			str++;
		else
			parse_op(ret, &str);
		if (prompt)
			break ;
	}
	{ t_token tmp = {.tt = TT_END, .start = 0, .len = 0}; deque_push_end(&ret->deqtok, &tmp); }
	return (prompt);
}
