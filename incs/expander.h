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

/* Expander functions */
void		expand_word(t_shell *state, t_ast_node *node,
				t_vec *args, bool keep_as_one);
char		*expand_word_single(t_shell *state, t_ast_node *curr);
void		expand_tilde_word(t_shell *state, t_ast_node *curr);
void		expand_env_vars(t_shell *state, t_ast_node *node);
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

/* Process substitution */
char		*expand_proc_sub(t_shell *state, t_ast_node *node);
void		cleanup_proc_subs(t_shell *state);
void		procsub_close_fds_parent(t_shell *state);

#endif