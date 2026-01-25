/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execution_private.h                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:05:22 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/26 00:47:46 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXECUTION_PRIVATE_H
# define EXECUTION_PRIVATE_H

# include "shell.h"
# include <stdbool.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
# include <errno.h>
# include "sh_error.h"
# include "env.h"
# include "redir.h"
# include <stddef.h>
# include <stdio.h>
# include "libft.h"
# include <readline/readline.h>
# include <string.h>
# include <sys/wait.h>
# include <signal.h>
# include "helpers.h"
# include "expander.h"
# include "sh_input.h"
# include "executor_types.h"

# define MSG_REDIR_DATA_ERR "%s: internal error: redirects \
							present but no redirect data\n"
# define MSG_AMBIGUOUS_REDIR "%s: ambiguous redirect\n"
# define MSG_INT_ERR_REDIR_IDX "%s: internal error: invalid redirect index %d\n"
# define NAME "shell"

typedef struct s_exec_child_ctx
{
	t_shell				*state;
	t_executable_node	*exe;			/* template exe node */
	t_executable_node	*curr_exe;		/* per-child node */
	int					(*pp)[2];		/* pointer to pipe pair */
	int					prev_infd;		/* preserved input fd */
	size_t				idx;			/* current child index */
	size_t				last_index;		/* last child index */
	t_vec_exe_res		*results;		/* results vector */
}	t_exec_child_ctx;

/* helper prototypes (ensure visible despite include-order */
char		*env_expand(t_shell *state, char *key);
void		free_tab(char **tab);
void		err_1_errno(t_shell *state, char *p1);
void		err_2(t_shell *state, char *p1, char *p2);
t_exe_res	execute_cmd_bg(t_shell *state, t_executable_node *exe,
				t_executable_cmd *cmd);
bool		check_is_a_dir(char *path, bool *enoent);
int			cmd_not_found(t_shell *state, char *cmd_name);
int			no_such_file_or_dir(t_shell *state,
				char *cmd_name, char *path_of_exe);
char		*exe_path(char **path_dirs, char *exe_name, int *perm_denied);
t_exe_res	execute_builtin_cmd_fg(t_shell *state,
				t_executable_cmd *cmd, t_executable_node *exe);
t_exe_res	execute_command(t_shell *state, t_executable_node *exe);
void		set_up_redir_pipeline_child(bool is_last, t_executable_node *exe,
				t_executable_node *curr_exe, int (*pp)[2]);
void		execute_pipeline_children(t_shell *state, t_executable_node *exe,
				t_vec_exe_res *results);
t_exe_res	execute_pipeline(t_shell *state, t_executable_node *exe);
t_exe_res	execute_simple_command(t_shell *state, t_executable_node *exe);
void		reap_background_children(void);
bool		should_execute(t_exe_res prev_status, t_tt prev_op);
size_t		find_next_separator(t_ast_node *node,
				size_t start, bool *found_amp);
t_exe_res	execute_range(t_shell *state, t_executable_node *exe,
				size_t start, size_t end);
t_exe_res	execute_range_background(t_shell *state, t_executable_node *exe,
				size_t start, size_t end);
t_exe_res	execute_simple_list(t_shell *state, t_executable_node *exe);
t_exe_res	execute_subshell(t_shell *state, t_executable_node *exe);
void		execute_top_level(t_shell *state);
t_exe_res	execute_tree_node(t_shell *state, t_executable_node *exe);
int			find_cmd_path(t_shell *state, char *cmd_name, char **path_of_exe);
t_exe_res	res_status(int status);
t_exe_res	res_pid(int pid);
void		exe_res_set_status(t_exe_res *res);
int			actually_run(t_shell *state, t_vec *args);
void		update_underscore_var(t_shell *state, t_executable_cmd *cmd);
void		set_up_redirection(t_shell *state, t_executable_node *exe);
void		apply_redirs_from_vec(t_shell *state, t_executable_node *exe);
void		apply_redirs_from_ast(t_shell *state, t_executable_node *exe);
int			handle_direct_path_error(t_shell *state, char *cmd_name,
				char **path_of_exe);
int			run_builtin_or_continue(t_shell *state, t_vec *args);
int			find_exe_path_wrapper(t_shell *state, char *cmd0, char **out_path);
void		try_exec_with_fallback(char *path_of_exe,
				t_vec *args, char **envp);
void		cleanup_after_exec_failure(t_vec *args,
				char *path_of_exe, char **envp);
int			map_errno_to_exit(void);

static inline t_executable_node	create_exe_node(int infd,
										int outfd,
										t_ast_node *node,
										bool modify_parent_context)
{
	return ((t_executable_node){
		.infd = infd,
		.outfd = outfd,
		.next_infd = -1,
		.node = node,
		.redirs = (t_vec){0},
		.modify_parent_context = modify_parent_context,
	});
}

#endif