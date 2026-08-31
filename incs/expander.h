/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:46:07 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:46:07 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Word expansion public API: variable expansion, command substitution,
   arithmetic, field splitting, glob, tilde, and parameter format
   (${v:-default} etc.).  expand_word is the main entry; the others are
   variants with different field-splitting / glob control. */

#ifndef EXPANDER_H
# define EXPANDER_H

# include "shell.h"
# include "helpers.h"
# include "env.h"

/* Loop state threaded through expand_simple_command.  i is the current
   child index; found_first tracks whether we have seen the command word
   (pre-command NAME=VALUE words come before it). */
typedef struct s_expander_simple_cmd
{
	size_t		i; /* current child node index */
	t_ast_node	*node; /* the simple command, for looking at siblings */
	t_ast_node	*curr; /* shorthand for children[i] */
	bool		found_first; /* true once the command word is seen */
	bool		export; /* true when expanding an `export` command */
	bool		in_db; /* true inside [[ ]]: no split/glob, case rules */
	bool		pat_next; /* [[ only: next word sits after ==/=/!= */
	int			exit_stat; /* running exit status for the command */
}	t_expander_simple_cmd;

/* expand_word_glob_ctl `flags` bits: keep the word as one field (no field
   splitting) and/or suppress pathname (glob) expansion. */
# define EW_KEEP_AS_ONE 1
# define EW_NO_GLOB 2

/* Expander functions */
void		expand_word(t_shell *state, t_ast_node *node,
				t_vec *args, bool keep_as_one);
void		expand_word_glob_ctl(t_shell *state, t_ast_node *node,
				t_vec *args, int flags);
void		expand_word_ro(t_shell *state, t_ast_node *src,
				t_vec *args, bool keep_as_one);
void		expand_word_assign_ro(t_shell *state, t_ast_node *src,
				t_vec *args);
void		expand_word_elem_ro(t_shell *state, t_ast_node *src,
				t_vec *args);
char		*expand_word_single(t_shell *state, t_ast_node *curr);
char		*expand_word_single_ro(t_shell *state, t_ast_node *curr);
void		expand_tilde_word(t_shell *state, t_ast_node *curr);
void		expand_cmd_substitutions(t_shell *state, t_ast_node *node);
void		expand_env_vars(t_shell *state, t_ast_node *node, bool split_ctx);
t_vec_nd	split_words(t_shell *state, t_ast_node *node);
t_string	word_to_string(t_ast_node node);
t_string	word_to_hrdoc_string(t_ast_node node);
t_env		assignment_to_env(t_shell *state, t_ast_node *node);
void		assignment_word_to_word(t_ast_node *node);
t_token_old	get_old_token(t_ast_node word);
int			expand_simple_command(t_shell *state, t_ast_node *node,
				t_executable_cmd *ret, t_vec_int *redirects);
int			redirect_from_ast_redir(t_shell *state, t_ast_node *curr,
				int *redir_idx);

/* Expand a $(...) or $((...)) at the start of `s`; push the result into
   `outbuf` and return the number of input chars consumed (0 if `s` is not a
   command/arith substitution). Used by the heredoc expander. */
int			expand_dollar_sub(t_shell *state, const char *s, int slen,
				t_string *outbuf);

/* Expand the body of a ${...} (the bytes between the braces): handles operator
   (${v:-w}), length (${#v}) and trim (${v#p}) forms, NULL otherwise. Used by
   both the word expander and the heredoc expander. */
char		*expand_param_format(t_shell *state, const char *s, int slen,
				bool dq);

/* Process substitution */
char		*expand_proc_sub(t_shell *state, t_ast_node *node);
void		cleanup_proc_subs(t_shell *state);
void		procsub_close_fds_parent(t_shell *state);
void		procsub_detach_all(t_shell *state);

/* Slab-backed allocator for short-lived argv word strings (word_slab.c).
   word_free() also accepts plain malloc'd pointers (libc-free fallback), so a
   command's argv (slab + malloc strings) frees uniformly. word_slab_push()
   force-selects the allocator for the current expansion context (1=slab for
   command argv, 0=malloc for values escaping to env) and returns the previous
   setting so callers save/restore around nesting. */
char		*word_strndup(const char *s, size_t n);
void		word_free(void *p);
int			word_slab_push(int on);
void		word_slab_teardown(void);

/* case-rules expansion (execute_case.c) shared with [[ ]] operands. */
char		*expand_case_word(t_shell *state, t_ast_node *node);
char		*expand_case_pattern(t_shell *state, t_ast_node *node);
bool		db_head_detect(t_ast_node *node);
int			process_db_child(t_shell *state, t_expander_simple_cmd *exp,
				t_executable_cmd *ret);

#endif