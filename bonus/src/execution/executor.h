/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:38:39 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:38:39 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXECUTOR_H
# define EXECUTOR_H

typedef enum s_res_t
{
	RES_OK,
	RES_FatalError,
	RES_MoreInput,
	RES_Init,
}	t_res_t;


typedef struct s_exe_res
{
	int	status;
	int	pid;
	bool	c_c;
}	t_exe_res;

typedef struct executable_cmd_s
{
	t_vec_env	pre_assigns;
	t_vec	argv;
	char		*fname;
}	t_executable_cmd;

typedef struct executable_node_s
{
	int			infd;
	int			outfd;
	int			next_infd;
	t_ast_node	*node;
	t_vec_int	redirs;
	bool		modify_parent_context;
}	t_executable_node;


void		execute_top_level(t_state *state);
t_exe_res	execute_pipeline(t_state *state, t_executable_node *exe);

void		set_up_redirection(t_state *state, t_executable_node *exe);
t_exe_res	execute_simple_command(t_state *state, t_executable_node *exe);
void		forward_exit_status(t_exe_res res);
t_exe_res	execute_command(t_state *state, t_executable_node *exe);
t_exe_res	execute_tree_node(t_state *state, t_executable_node *exe);
t_exe_res	execute_simple_list(t_state *state, t_executable_node *exe);
t_string	word_to_string(t_ast_node node);
t_string	word_to_hrdoc_string(t_ast_node node);
void		set_cmd_status(t_state *state, t_exe_res res);

# endif