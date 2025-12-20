/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:36:03 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/19 17:36:39 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <string.h>
//#include "libft.h"

typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_REDIR_APPEND,
    TOKEN_HEREDOC,
    TOKEN_EOF
}	t_token_type;

typedef struct s_dynamic_str
{
	char	*str;
	size_t	len;
	size_t	dyn_str_capacity;
}	t_dynamic_str;

typedef struct s_token
{
	t_token_type	type;
	t_dynamic_str	value;
}	t_token;

typedef struct s_token_vector
{
	t_token **tokens;
	size_t	count;
	size_t	capacity;
}	t_token_vector;

typedef struct s_lexer_state
{
	const char		*input;
	t_token_vector	*vector;
	t_dynamic_str	*current_token_str;
	size_t			*i;
}	t_lexer_state;

t_token_vector	*vector_init(size_t initial_capacity);
int				vector_append(t_token_vector *vector, t_token *token);
void			vector_free(t_token_vector *vector);
int				add_token_to_vector(t_token_vector *vector, t_dynamic_str *d_str, t_token_type type);

int 			d_str_init(t_dynamic_str *d_str, size_t dyn_initial_capacity);
int				d_str_append_char(t_dynamic_str *d_str, char c);
int				d_str_append_str(t_dynamic_str *d_str, const char *str);
void			d_str_reset(t_dynamic_str *d_str);
void 			d_str_free(t_dynamic_str *d_str);

t_token_vector	*lexer(const char *input);

int				is_pipe(char c);
int				is_redirect(char c);
int				is_quote(char c);
int				is_special_char(char c);
int				is_whitespace(char c);

void	handle_redirect(t_lexer_state *state);
void	handle_word(t_lexer_state *state);
void	handle_pipe(t_lexer_state *state);
void	handle_whitespace(t_lexer_state *state);
void	finalize_word(t_lexer_state *state);
int		handle_quotes(t_lexer_state *state);

#endif