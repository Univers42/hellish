/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sh_alias.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Alias subsystem public types and API.
   Aliases are stored in a t_hash keyed on the alias name.  Expansion
   happens at the input level (before lexing) so aliases work identically
   to bash: they replace the first word of a simple command.  Recursive
   expansion is bounded by ALIAS_MAX_DEPTH to prevent infinite loops. */

#ifndef SH_ALIAS_H
# define SH_ALIAS_H

# include "libft.h"

/* Maximum alias expansion recursion depth (alias a='b'; alias b='a').
   64 matches the bash limit; beyond this we stop expanding and leave
   the string as-is rather than looping forever. */
# define ALIAS_MAX_DEPTH 64

/* One alias definition.  Both strings are heap-allocated and owned
   by this entry (freed in alias_remove / alias_table_free). */
typedef struct s_alias_entry
{
	char	*name; /* the alias name (e.g. "ll") */
	char	*value; /* the replacement text (e.g. "ls -la") */
}	t_alias_entry;

struct	s_shell;

/* Alias scanning happens on the raw input string BEFORE the tokenizer,
   exactly like bash: an unquoted word in command position that names an
   alias is textually replaced, the replacement is rescanned (stack of
   sources; a name already on the stack is never re-expanded), and a value
   ending in a blank makes the following word eligible too.  Quoted spans,
   $(...) bodies, comments and here-doc bodies are copied through
   untouched.  ALIAS_SCAN_DEPTH bounds the source stack; ALIAS_HD_MAX
   bounds the pending here-doc delimiter queue. */
# define ALIAS_SCAN_DEPTH 16
# define ALIAS_HD_MAX 8

typedef struct s_ascan_src
{
	const char	*s;
	size_t		pos;
	char		*name;
	bool		blank_end;
}	t_ascan_src;

typedef struct s_ascan
{
	t_ascan_src	src[ALIAS_SCAN_DEPTH];
	int			depth;
	t_vec		out;
	t_hash		*aliases;
	bool		cmd_pos;
	bool		chk_next;
	bool		wstart;
	bool		after_redir;
	int			pend_hd;
	char		*hd_delim[ALIAS_HD_MAX];
	bool		hd_strip[ALIAS_HD_MAX];
	int			hd_n;
}	t_ascan;

void	alias_table_init(t_hash *aliases);
void	alias_table_free(t_hash *aliases);
int		alias_set(t_hash *aliases, const char *name, const char *value);
char	*alias_get(t_hash *aliases, const char *name);
int		alias_remove(t_hash *aliases, const char *name);
void	alias_print_all(t_hash *aliases);
int		alias_print_one(t_hash *aliases, const char *name);
bool	alias_table_empty(t_hash *aliases);

char	*alias_scan_line(t_hash *aliases, const char *input);
void	alias_scan_update(struct s_shell *state);
bool	asc_push(t_ascan *a, const char *name, size_t nlen, const char *val);
void	asc_pop(t_ascan *a);
bool	asc_active(t_ascan *a, const char *w, size_t len);
void	asc_word(t_ascan *a);
void	asc_op(t_ascan *a);
void	asc_newline(t_ascan *a);
void	asc_comment(t_ascan *a);
void	asc_hd_delim(t_ascan *a, const char *w, size_t len);
void	asc_hd_drop(t_ascan *a);
void	asc_hd_body(t_ascan *a, t_ascan_src *s);
void	asc_op_extra(t_ascan *a, t_ascan_src *s);
void	asc_scan_blank(t_ascan *a, t_ascan_src *s, char c);
void	asc_emit_sep(t_ascan *a);
size_t	asc_span_dollar(const char *p);
size_t	asc_span_btick(const char *p);
size_t	asc_span_quote(const char *p);
bool	asc_digits_redir(const char *w, size_t len);
bool	asc_is_assign(const char *w, size_t len);
bool	asc_kw_cmdnext(const char *w, size_t len);
bool	asc_kw_noncmd(const char *w, size_t len);

#endif
