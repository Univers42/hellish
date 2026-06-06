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

/* Lexer public API and data structures.
   tokenizer() is the entry point: it scans a NUL-terminated string and
   fills a t_deque_tok with t_token values.  Tokens are NOT copied -- they
   are char* slices into the original string (or into allocated expansions
   for the few cases that need it).  The caller must keep the string alive
   at least as long as the token deque. */

#ifndef LEXER_H
# define LEXER_H

# include "libft.h"
# include "token.h"

struct	s_shell;

/* A deque of tokens plus the closing char we are looking for in
   multi-line input (e.g. ')' after '(' makes us ask for more input). */
typedef struct s_deque_tok
{
	t_deque		deqtok;       /* the token deque */
	char		looking_for;  /* '\0' normally; ')' / '}' if incomplete */
}	t_deque_tok;

/* One entry in the operator string-to-token-type table.  The table is
   sorted longest-first so the lexer can match greedily (>> before >). */
typedef struct s_op_map
{
	char	*str; /* operator string e.g. ">>" */
	t_tt	t;   /* token type e.g. TT_APPEND */
}	t_op_map;

char		*tokenizer(char *str, t_deque_tok *ret);
int			advance_dquoted(char **str);
int			advance_squoted(char **str);
int			advance_backtick(char **str);
int			advance_brace_param(char **str);
void		free_all_state(struct s_shell *state);
void		print_tokens(t_deque_tok *tokens);
char		*tt_to_str(t_tt tt);
char		*tt_to_str_p2(t_tt tt);
const char	*token_color(t_tt tt);
void		print_visible_lexeme_noquotes(t_token *t);
bool		is_special_char(char c);
bool		is_space(char c);
bool		is_word_boundary(const char *s);
char		*parse_lexeme(t_deque_tok *tokens, char **str);
void		parse_op(t_deque_tok *tokens, char **str);
void		dbracket_toggle(const char *str, int *in_db);
int			emit_dbracket_word(char **str, t_deque_tok *ret);
void		reclassify_keywords(t_deque_tok *tokens);

/* helpers used by debug/tables printing */
size_t		visible_lexeme_len(t_token *t);
size_t		num_digits(size_t v);
void		compute_columns(t_deque_tok *tokens,
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