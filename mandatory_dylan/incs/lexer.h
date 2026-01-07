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
	TT_WORD,
	TT_REDIRECT_LEFT,	// <
	TT_REDIRECT_RIGHT,	// >
	TT_APPEND,			// >>
	TT_PIPE,			// |
	TT_HEREDOC,			// << | <<-
	TT_NEWLINE,			// '\n'
	TT_BRACE_LEFT,		// (
	TT_BRACE_RIGHT,		// )
	TT_AND,				// &&
	TT_OR,				// ||
	TT_SEMICOLON,		// ; or &
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
}	t_token;

typedef struct s_op_map
{
	const char	*str;
	t_tt		t;
}	t_op_map;

static inline int	advance_dquoted(char **str)
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

static inline int	advance_squoted(char **str)
{
	ft_assert(**str == '\'');
	(*str)++;
	while (**str && **str != '\'')
	{
		(*str)++;
	}
	if (**str != '\'')
		return (1);
	(*str)++;
	return (0);
}

char	*tokenizer(char *str, t_deque_tt *ret);
#endif