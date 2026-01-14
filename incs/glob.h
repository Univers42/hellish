/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:00:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:00:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GLOBBER_H
# define GLOBBER_H

# include "libft.h"
# include "lexer.h"

typedef enum e_glob
{
	G_PATTERN,
	G_SLASH,
	G_ASTERISK
}	t_glob_tt;

typedef struct s_glob
{
	t_glob_tt	ty;
	char		*start;
	int			len;
}	t_glob;

typedef t_vec	t_vec_glob;
typedef t_vec	t_string;

typedef struct s_dir_matcher
{
	DIR				*dir;
	t_vec			*args;
	t_vec_glob		glob;
	char			*path;
	size_t			offset;
}	t_dir_matcher;

t_vec_glob	word_to_glob(t_ast_node word);
t_vec		expand_word_glob(t_ast_node word);
t_vec		expand_word_glob(t_ast_node word);
t_vec_glob	word_to_glob(t_ast_node word);


void		match_dir(t_vec *args,
				t_vec_glob glob, char *path, size_t offset);
size_t		matches_pattern(char *name,
				t_vec_glob patt, size_t offset, bool first);
size_t		matches_pattern(char *name, t_vec_glob patt,
				size_t offset, bool first);
void		ft_quicksort(t_vec *vec);
void		get_next_path(t_string *next_path, char *path, char *fname);
char		*get_curr_path(char *path);
int			process_dir(t_dir_matcher matcher);
void		match_dir(t_vec *args, t_vec_glob glob, char *path, size_t offset);
void		tokenize_star_glob(t_vec_glob *ret, t_token t, int *i);
bool		star_expandable(t_tt tt);
void		tokenize_pattern(t_vec_glob *ret, t_token t, int *i);
void		tokenize_word_glob(t_vec_glob *ret, t_token t);

#endif