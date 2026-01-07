/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 17:20:20 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 17:20:20 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_H
# define LEXER_H

# include "common.h"

typedef enum e_tt
{
	TT_NONE = 0,
	TT_WORD,			//asdgfgdkgjl
	TT_REDIRECT_LEFT,	// <
	TT_REDIRECT_RIGHT,	// >
	TT_APPEND,			// >>
	TT_PIPE,			// |
	TT_HEREDOC,			// << | <<-
	TT_NEWLINE,			// '\n'
	TT_END,
	TT_SQWORD,
	TT_DQWORD,
	TT_ENVVAR,
	TT_DQENVVAR
}	t_tt;

typedef struct s_token_old
{
	bool	present;
	char	*start;
	size_t	len;
}	t_token_old;

typedef struct s_token
{
	char		*start;
	size_t		len;
	t_tt		tt;
	t_token_old	full_word;
	bool		allocated;
}t_token;

typedef struct s_op_map
{
	const char	*str;
	t_tt		t;
}	t_op_map;

static inline bool	advance_dquoted(char **str)
{
	bool	prev_bs;

	ft_assert(**str == '\"');
	(*str)++;
	prev_bs = false;
	while (**str && (**str != '\"' || prev_bs))
	{
		prev_bs = **str == '\\' && !prev_bs;
		(*str)++;
	}
	if (**str != '\"')
		return (1);
	(*str)++;
	return (0);
}

static inline bool	advance_squoted(char **str)
{
	ft_assert(**str == '\'');
	(*str)++;
	while (**str && **str != '\'')
		(*str)++;
	if (**str != '\'')
		return (1);
	(*str)++;
	return (0);
}

static inline void advance_bs(char **str)
{
	ft_assert(**str == '\\');
	if ((*str)[1])
		*str += 1;
	*str += 1;
}

static inline char *parse_word(t_deque *tokens, char **str)
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
				return (token->looking_for = '\'', SQUOTE_PROMPT);
		}
		else if (**str == '"')
		{
			if (advance_dquoted(str))
				return (tokens->looking_for = '"', DQUOTE_PROMPT);
		}
		else
			break ;
	}
	deque_push_end(tokens, (t_token){.start = start, .len = *str - start, .tt = TT_WORD});
	return (0);
}

static inline int	longest_matching_str(t_op_map *needles, char *haystack)
{
	int		max_idx;
	int		max;
	int		i;

	max_idx = -1;
	max = -1;
	i = 0;
	while (needles[i].str)
	{
		int nlen = (int)strlen(needles[i].str);
		if (nlen > max && strncmp(needles[i].str, haystack, nlen) == 0)
		{
			max_idx = i;
			max = nlen;
		}
		i++;
	}
	return (max_idx);
}

static inline void parse_op(t_deque *tokens, char **str)
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
	operators[6] = (t_op_map){"<", TT_REDIRECT_LEFT};
	operators[7] = (t_op_map){">", TT_REDIRECT_RIGHT};
	operators[8] = (t_op_map){"&&", TT_AND};
	operators[9] = (t_op_map){"&", TT_SEMICOLON};
	operators[10] = (t_op_map){"||", TT_OR};
	operators[11] = (t_op_map){";", TT_SEMICOLON};
	operators[12] = (t_op_map){0, TT_END};

	start = *str;
	op_idx = longest_matching_str(operators, *str);
	ft_assert(op_idx != -1);
	*str += strlen(operators[op_idx].str);
	deque_push_end(tokens, & (t_token){
		.start = start,
		.len = (size_t)(*str - start),
		.tt = operators[op_idx].t,
		.allocated = false
	});
}

static inline char	*tokenizer(char *str, t_deque *ret)
{
	char	*prompt;

	prompt = 0;
	/* reset deque state using helper */
	if (ret)
		deque_clear(ret, NULL);
	while (str && *str)
	{
		if (*str == '\'' || *str == '"' || *str == '$' || !is_special_char(*str))
			prompt = parse_word(ret, &str);
		else if (*str == '\n')
		{
			deque_push_end(ret, (t_token){.start = str, .len = 1, .tt = TT_NEWLINE});
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


#endif