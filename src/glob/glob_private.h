/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_private.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 10:18:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 13:13:23 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GLOB_PRIVATE_H
# define GLOB_PRIVATE_H

# include "ft_glob.h"
# include "shell.h"
# include "lexer.h"
# include "expander.h"
# include <dirent.h>
# include <stdlib.h>
# include <string.h>

/* struct describing POSIX classes */
struct s_classes
{
	const char	*pattern;
	int			plen;
	const char	*chars;
};

typedef struct s_bracket_ctx
{
	const char	*start;
	int			len;
	char		*buf;
	int			buf_pos;
	int			i;
}	t_bracket_ctx;

typedef struct s_glob_match_ctx
{
	const char	*name;
	t_vec_glob	*pattern;
	size_t		offset;
	bool		is_first;
}	t_glob_match_ctx;

// Helper: context for tokenizer state
typedef struct s_tokenizer_ctx
{
	t_vec_glob	*ret;
	const char	*pattern;
	int			*i;
	int			len;
	bool		quoted;
}	t_tokenizer_ctx;

typedef struct s_bracket_parse_ctx
{
	const char	*s;
	int			content_start;
	int			i;
	int			max_len;
	int			flags;
}	t_bracket_parse_ctx;

typedef struct s_bracket_ctx	t_bracket_ctx;

// Add prototype for the singleton accessor
static inline const struct s_classes	*get_classes_singleton(void)
{
	static const struct s_classes	classes[] = {
	{"[:alnum:]", 9, "0123456789ABCDEFGHIJKLMNO"
		"PQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"},
	{"[:alpha:]", 9, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"},
	{"[:digit:]", 9, "0123456789"},
	{"[:lower:]", 9, "abcdefghijklmnopqrstuvwxyz"},
	{"[:upper:]", 9, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
	{"[:space:]", 9, " \t\n\r\f\v"},
	{"[:blank:]", 9, " \t"},
	{"[:punct:]", 9, "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"},
	{"[:xdigit:]", 10, "0123456789ABCDEFabcdef"},
	{"[:cntrl:]", 9, "\001\002\003\004\005\006\007\010"
		"\011\012\013\014\015"
		"\016\017\020\021\022\023\024\025\026\027\030"
		"\031\032\033\034\035\036\037\177"},
	{"[:graph:]", 9, "!\"#$%&'()*+,-./0123456789:;<=>?@"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^"
		"_`abcdefghijklmnopqrstuvwxyz{|}~"},
	{"[:print:]", 9, " !\"#$%&'()*+,-./0123456789:;"
		"<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]"
		"^_`abcdefghijklmnopqrstuvwxyz{|}~"},
	{NULL, 0, NULL}};

	return (classes);
}

/* single-definition: actual array provided in glob_classes.c */
// extern const struct s_classes classes[];
char		*glob_expand_bracket(const char *start, int len, int *out_len);
int			expand_range(char start, char end, char *buf, int buf_pos);
int			check_posix_class(const char *s, int len, char *buf, int *buf_pos);
bool		glob_char_in_class(char c, t_glob *bracket);
int			match_literal(const char *name, t_glob *g);
int			match_question(const char *name, bool is_first);
int			match_bracket(const char *name, t_glob *g, bool is_first);
bool		match_asterisk_recursive(const char *name, t_vec_glob *pattern,
											size_t offset, bool is_first);
bool		glob_match_at(const char *name, t_vec_glob *pattern, size_t offset);
bool		glob_match(const char *name, t_vec_glob *pattern);
size_t		match_g_asterisk(char *name, t_vec_glob patt, size_t offset,
											bool first);
size_t		match_g_question(char *name, t_vec_glob patt, size_t offset,
											bool first);
size_t		match_g_bracket(char *name, t_vec_glob patt, size_t offset,
											bool first);
size_t		match_g_literal(char *name, t_vec_glob patt, size_t offset,
											bool first);
size_t		matches_pattern(char *name, t_vec_glob patt,
											size_t offset, bool first);
void		tokenize_asterisk(t_tokenizer_ctx ctx);
void		tokenize_question(t_tokenizer_ctx ctx);
void		tokenize_literal_n(t_tokenizer_ctx ctx, bool can_glob, int force_n);
void		tokenize_literal(t_tokenizer_ctx ctx);
void		tokenize_ast_token(t_vec_glob *ret, t_token t);
int			process_dir(t_dir_matcher matcher);
void		match_dir(t_vec *args, t_vec_glob glob, char *path, size_t offset);
void		free_str_elem(void *el);
char		*get_curr_path(char *path);
bool		finished_pattern(t_vec_glob patt, size_t offset);
bool		glob_is_special(char c);
void		glob_free_tokens(t_vec_glob *tokens);
int			parse_bracket(const char *s, int max_len, t_glob *g);
void		get_next_path(t_string *next_path, char *path, char *fname);
char		peek_bracket(const t_bracket_ctx *ctx, int offset);
char		peek_elem(const void *arr, int base, int offset, size_t elem_size);
void		set_last_elem(char *arr, int len, char val);
void		consume_range(t_bracket_ctx *ctx, int range_count, int i_delta);
int			handle_invalid_range(t_bracket_ctx *ctx);
int			handle_posix_class(t_bracket_ctx *ctx);
int			handle_backslash(t_bracket_ctx *ctx);
void		handle_leading_bracket(t_bracket_ctx *ctx);
char		*expand_leading_bracket_special(const char *start, int *out_len);
bool		is_leading_bracket_special(const char *start, int len);
void		handle_slash_token(t_tokenizer_ctx ctx);
void		handle_asterisk_token(t_tokenizer_ctx ctx);
void		handle_question_token(t_tokenizer_ctx ctx);
void		handle_bracket_token(t_tokenizer_ctx ctx);
void		handle_literal_token(t_tokenizer_ctx ctx);

#endif
