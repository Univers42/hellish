/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 15:22:58 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/22 17:21:01 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <string.h>
//#include "libft.h"

typedef enum e_tok_type
{
    TOK_WORD,
	TOK_DQ,
	TOK_SQ,
    TOK_PIPE,
    TOK_LESS,
    TOK_REDIR_OUT,
    TOK_REDIR_APPEND,
    TOK_HEREDOC,
	TOK_DOLLAR,
    TOK_EOF
}	t_tok_type;


hello || <("something")

[[hello][[TOKEN_LOGIALC_OR][||]][<][(]["something"][)][][][][][][]]

# define ALLOC_MINI 64

t_token	**tokenize(char *input)
{
	char	*curr;
	t_token	**map_toks;

	map_toks = NULL;
	map(&map_toks, sizeof(t_token **) * ALLOC_MINI);
	while (*input)
	{
		// skip traililng space
		// if a character is discovered i's time to find whicch type of token it is
		curr = input;
		switch(curr)
		{
			case ft_strncmp(curr, "||", 2):
				(add_to_map((t_token *)make_token(TOK_LOGICAL_OR, "||"), &map_toks));
			case ft_strncmp(curr, "|", 1):
				(add_to_map((t_token *)make_token(TOK_LOGICAL_OR, "|"), &map_toks));
			case ft_strncmp(curr, "<", 1):
				(add_to_map((t_token *)make_token(TOK_LOGICAL_OR, "<"), &map_toks));
			case ft_strncmp(curr, "<<", 2):
				(add_to_map((t_token *)make_token(TOK_LOGICAL_OR, "<<"), &map_toks));
			case ft_strncmp(curr, ">>", 2):
				(add_to_map((t_token *)make_token(TOK_LOGICAL_OR, ">>"), &map_toks));
			case ft_strncmp(curr, "||", 2):
				(add_to_map((t_token *)make_token(TOK_LOGICAL_OR, "||"), &map_toks));	
		}
	}
}

typedef struct s_d_str
{
	char	*str;
	size_t	len;
	size_t	d_str_cap;
}	t_d_str;

typedef struct s_tok
{
	t_tok_type	type;
	t_d_str		value;
}	t_tok;

typedef struct s_tok_vec
{
	t_tok	**tokens;
	size_t	count;
	size_t	capacity;
}	t_tok_vec;

typedef struct s_lexer_st
{
	const char	*input;
	t_tok_vec	*vec;
	t_d_str		*curr_tok_str;
	size_t		*i;
}	t_lexer_st;

t_tok_vec	*vector_init(size_t initial_capacity);
int			vector_append(t_tok_vec *vector, t_tok *token);
void		vector_free(t_tok_vec *vector);
int			add_token_to_vec(t_tok_vec *vector, t_d_str *d_str, t_tok_type type);

int 		d_str_init(t_d_str *d_str, size_t dyn_initial_capacity);
int			d_str_append_char(t_d_str *d_str, char c);
int			d_str_append_str(t_d_str *d_str, const char *str);
void		d_str_reset(t_d_str *d_str);
void 		d_str_free(t_d_str *d_str);

t_tok_vec	*lexer(const char *input);

int			is_pipe(char c);
int			is_redirect(char c);
int			is_quote(char c);
int			is_special_char(char c);
int			is_whitespace(char c);

void		handle_redirect(t_lexer_st *state);
void		handle_pipe(t_lexer_st *state);
void		handle_whitespace(t_lexer_st *state);
void		finalize_word(t_lexer_st *state);
int			handle_quotes(t_lexer_st *state);

#endif
