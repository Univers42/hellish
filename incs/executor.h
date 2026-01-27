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

# include "public/executor_types.h"
# include "alias.h"

/* forward declarations to avoid heavy includes */
typedef struct s_shell		t_shell;
typedef struct s_ast_node	t_ast_node;

typedef struct executable_cmd_s
{
	t_vec_env	pre_assigns;
	t_vec		argv;
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

void				execute_top_level(t_shell *state);
t_execution_state	execute_pipeline(t_shell *state, t_executable_node *exe);
void				set_up_redirection(t_shell *state, t_executable_node *exe);
t_execution_state	execute_simple_command(t_shell *state, t_executable_node *exe);
void				forward_exit_status(t_execution_state res);
t_execution_state	execute_command(t_shell *state, t_executable_node *exe);
t_execution_state	execute_tree_node(t_shell *state, t_executable_node *exe);
t_execution_state	execute_simple_list(t_shell *state, t_executable_node *exe);
t_string			word_to_string(t_ast_node node);
t_string			word_to_hrdoc_string(t_ast_node node);
void				set_cmd_status(t_shell *state, t_execution_state res);
t_execution_state	res_status(int status);
t_execution_state	res_pid(int pid);
void				exe_res_set_status(t_execution_state *res);
int					find_cmd_path(t_shell *state, char *cmd_name, char **path_of_exe);
int					expand_simple_command(t_shell *state, t_ast_node *node,
						t_executable_cmd *ret, t_vec_int *redirects);
int					(*builtin_func(char *name))(t_shell *state, t_vec argv);

#endif