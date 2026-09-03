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
	t_deque		deqtok; /* the token deque */
	char		looking_for; /* '\0' normally; ')' / '}' if incomplete */
	char		*base; /* tokenize base: deque slots hold offsets from here */
}	t_deque_tok;

/* Push a freshly-lexed t_token into the deque as an 8-byte offset slot. The
   whole lexer builds t_tokens and hands them here; the (start -> off) packing
   lives in exactly this one spot so no push site has to know about it. */
static inline void	push_ltok(t_deque_tok *d, t_token t)
{
	t_ltoken	l;

	l = tok2ltok(t, d->base);
	deque_push_end(&d->deqtok, &l);
}

/* Pop the front deque slot and lift it to a full AST token against the
   deque base. The lexeme-offset rebuild lives here so parser call sites stay
   short and never juggle the base pointer by hand. */
static inline t_token	pop_tok(t_deque_tok *d)
{
	return (ltok2tok(*(t_ltoken *)deque_pop_start(&d->deqtok), d->base));
}

char		*tokenizer(char *str, t_deque_tok *ret);

/* Fixed-length keyword compare for the hot lexer/parser paths. ft_strncmp is a
   non-inlined libft function whose body calls __builtin_strncmp with a RUNTIME
   n -- which GCC lowers to libc's AVX2 strncmp, so comparing a 2-5 byte keyword
   pays a call into a routine tuned for long strings. Keyword matching is ~20%
   of parse50k instructions purely from that overhead. Here n and kw are always
   compile-time constants at the call site, so GCC unrolls this to a couple of
   byte loads + compares, fully inlined, no call. Callers length-gate first, so
   reading n bytes of s is in bounds. */
static inline bool	kw_eq(const char *s, const char *kw, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		if (s[i] != kw[i])
			return (false);
		i++;
	}
	return (true);
}

int			glob_qual_ahead(const char *start, const char *at);
bool		zsh_eqsub_break(const char *start, const char *at);
bool		skip_noise(char **str);
char		*tokenize_step(char **str, t_deque_tok *ret, int *in_db);
char		*lex_line(char *base, char **str, t_deque_tok *ret, int *in_db);
int			advance_dquoted(char **str);
int			advance_squoted(char **str);
int			advance_ansic(char **str);
int			advance_backtick(char **str);
int			advance_brace_param(char **str, int in_dq);
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
bool		db_newline_skippable(t_deque_tok *ret, const char *after);
void		db_track_regex(t_deque_tok *ret, int *in_db);
int			db_regex_word(char **str, t_deque_tok *ret, int *in_db);
int			*db_front_cell(void);
int			db_front_group(const char *at);
void		reclassify_keywords(t_deque_tok *tokens, bool zsh);
long		*zsh_brace_cell(void);

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