/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Per-command completion providers and the Makefile target provider.
   Included only by src/completion/complete_provider.c and
   src/completion/complete_make*.c. */

#ifndef COMPLETE_MAKE_H
# define COMPLETE_MAKE_H

# include "libft.h"
# include "shell.h"
# include "zle.h"
# include <stddef.h>
# include <sys/stat.h>

/* The line under the cursor, as a cursor over the words that precede the
   word being completed: [pos, end) is what has not been read yet. */
typedef struct s_pv_line
{
	const char	*l;
	size_t		pos;
	size_t		end;
}	t_pv_line;

typedef char	**(*t_prov_fn)(t_shell *st, const char *text, t_pv_line *a);

typedef struct s_provider
{
	const char	*cmd;
	t_prov_fn	fn;
}	t_provider;

/* One Makefile scan: where targets go, how deep the include is (0 = the
   Makefile itself, 1 = a file it includes), and the depth of define. */
typedef struct s_mk_scan
{
	t_vec	*out;
	int		level;
	int		def;
}	t_mk_scan;

# define MK_MAX 4194304

/* The nanosecond mtime field is st_mtim on POSIX 2008, st_mtimespec on
   Darwin (same split as builtin_test_posix.c). */
# ifdef __APPLE__
#  define ST_MTIM st_mtimespec
# else
#  define ST_MTIM st_mtim
# endif

size_t	pc_cmd_off(int start);
size_t	prov_word(t_pv_line *p, const char **w);
size_t	prov_base(const char *w, size_t n);
char	**comp_provider_try(const char *text, int start, int end);
char	**comp_make(t_shell *st, const char *text, t_pv_line *a);
char	*rl_dup(const char *s);
t_vec	*mk_load(t_shell *st);
void	mk_cache_free(t_shell *st);
char	*mk_slurp(const char *path, size_t *len);
void	mk_scan(t_mk_scan *m, const char *buf, size_t len);
void	mk_line(t_mk_scan *m, char *l);
int		mk_blank(char c);
int		mk_word_is(const char *l, const char *w);
void	mk_rule(t_mk_scan *m, char *l);
void	mk_add(t_vec *v, const char *w, size_t n);
int		mk_directive(t_mk_scan *m, char *l);
int		mk_pick(struct stat *sb, const char **name);
void	mk_clear(t_make_cache *c);
int		mk_same(t_make_cache *c, const char *name, struct stat *sb);
void	mk_stamp(t_make_cache *c, struct stat *sb);

#endif
