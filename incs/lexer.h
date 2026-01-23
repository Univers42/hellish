/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:20:12 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:20:12 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_H
# define LEXER_H

# include "libft.h"
# include "token.h"

struct	s_shell;
typedef struct s_deque_tt
{
	t_deque		deqtok;
	char		looking_for;
}	t_deque_tt;

typedef struct s_op_map
{
	char	*str;
	t_tt	t;
}	t_op_map;

char		*tokenizer(char *str, t_deque_tt *ret);
int			advance_dquoted(char **str);
int			advance_squoted(char **str);
void		free_all_state(struct s_shell *state);
void		print_tokens(t_deque_tt *tokens);
char		*tt_to_str(t_tt tt);
char		*tt_to_str_p2(t_tt tt);
const char	*token_color(t_tt tt);
void		print_visible_lexeme_noquotes(t_token *t);
bool		is_special_char(char c);
bool		is_space(char c);
bool		is_word_boundary(const char *s);
char		*parse_lexeme(t_deque_tt *tokens, char **str);
void		parse_op(t_deque_tt *tokens, char **str);

/* helpers used by debug/tables printing */
size_t		visible_lexeme_len(t_token *t);
size_t		num_digits(size_t v);
void		compute_columns(t_deque_tt *tokens,
				size_t *w_name, size_t *w_len, size_t *w_lexeme);
void		print_table_header(size_t w_name, size_t w_len, size_t w_lexeme);
void		print_table_footer(size_t w_name, size_t w_len, size_t w_lexeme);

/* singletons provided in singletons.c */
const char	**get_tt_names(void);
t_hash		*get_color_map(void);
void		advance_bs(char **str);
int			create_token_consume(char *start, int fd_len,
				t_tt tt, t_token *out);
int			check_fd_redirect(char *str, t_token *out);

#endif