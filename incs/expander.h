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

#ifndef EXPANDER_H
# define EXPANDER_H

# include "shell.h"
# include "helpers.h"
# include "env.h"

/* Expander types */
typedef struct s_expander_simple_cmd
{
	size_t		i;
	t_ast_node	*curr;
	bool		found_first;
	bool		export;
	int			exit_stat;
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

/* Process substitution */
char		*expand_proc_sub(t_shell *state, t_ast_node *node);
void		cleanup_proc_subs(t_shell *state);
void		procsub_close_fds_parent(t_shell *state);

#endif