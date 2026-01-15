/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GLOB_H
# define GLOB_H

# include <stdbool.h>
# include <stddef.h>
# include "libft.h"
# include "lexer.h"

/*
** POSIX Glob Pattern Types
*/
typedef enum e_glob_type
{
	G_LITERAL,
	G_ASTERISK,
	G_QUESTION,
	G_BRACKET,
	G_SLASH,
}	t_glob_type;

/*
** Bracket expression flags for character classes
*/
typedef enum e_bracket_flags
{
	BRACKET_NONE = 0,
	BRACKET_NEGATED = 1,
	BRACKET_HAS_RANGE = 2,
}	t_bracket_flags;

/*
** Glob token structure
*/
typedef struct s_glob
{
	t_glob_type		ty;
	const char		*start;
	int				len;
	int				flags;
	char			*char_set;
	int				char_set_len;
}	t_glob;

/* Vector of glob tokens */
typedef t_vec	t_vec_glob;

/* Forward declarations for project types */
typedef struct s_ast_node	t_ast_node;
typedef struct s_token		t_token;

/*
** GLOB MOTOR API
*/
t_vec_glob	glob_tokenize(const char *pattern, int len, bool quoted);
void		glob_free_tokens(t_vec_glob *tokens);
bool		glob_match(const char *name, t_vec_glob *pattern);
bool		glob_match_at(const char *name, t_vec_glob *pattern, size_t offset);
bool		glob_char_in_class(char c, t_glob *bracket);
char		*glob_expand_bracket(const char *start, int len, int *out_len);

/*
** GLOB INTEGRATION API
*/
t_vec_glob	word_to_glob(t_ast_node word);
t_vec		expand_word_glob(t_ast_node word);
void		match_dir(t_vec *args, t_vec_glob glob, char *path, size_t offset);
size_t		matches_pattern(char *name, t_vec_glob patt, size_t offset, bool first);
bool		finished_pattern(t_vec_glob patt, size_t offset);
bool		star_expandable(t_tt tt);

#endif