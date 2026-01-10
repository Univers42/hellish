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

# include "common.h"

typedef struct s_expander_simple_cmd
{
	bool		found_first;
	bool		export;
	int			exit_stat;
	t_ast_node	*curr;
	size_t		i;
}	t_expander_simple_cmd;

void		expand_word(t_state *state, t_ast_node *node,
				t_vec *args, bool keep_as_one);
int			expand_simple_command(t_state *state, t_ast_node *node,
				t_executable_cmd *ret, t_vec_int *redirects);
int			redirect_from_ast_redir(t_state *state,
				t_ast_node *curr, int *redir_idx);
t_vec_nd	split_words(t_state *state, t_ast_node *node);

t_env		assignment_to_env(t_state *state, t_ast_node *node);
void		assignment_word_to_word(t_ast_node *node);
t_token_old	get_old_token(t_ast_node word);

char		*expand_word_single(t_state *state, t_ast_node *curr);
void		expand_env_vars(t_state *state, t_ast_node *node);
void		expand_tilde_word(t_state *state, t_ast_node *curr);

# endif