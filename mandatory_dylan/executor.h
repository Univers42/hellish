/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 22:51:05 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 22:51:05 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXECUTOR_H
# define EXECUTOR_H

# include "common.h"

typedef struct s_exe_cmd
{
	t_env		pre_assigns;
	t_strings	argv;
	char		*fname;
}	t_exe_cmd;

typedef struct s_exe_node
{
	int			infd;
	int			outf;
	int			next_infd;
	t_ast_node	*node;
	t_ints		redirs;
	bool		modify_parent_context;cd
}	t_exe_node;

typedef s_exe_st
{
	int	code;
	int	pid;
	bool	c_c;
}	t_exe_ex_code;

// take t_exe_ex_code as context buffer
typedef t_vec t_exe_res;
typedef struct stat t_stat;

static inline t_exe_st	exe_status(int st)
{
	return ((t_exe_st){.status = st, .pid = -1});
}

static inline t_exe_st	res_pid(int pid)
{
	return ((t_exe_st){.status = -1, .pid = pid});
}

static inline exe_res_set_status(t_exe_st *st_ex)
{
	int	status;

	if (st_ex->status != 1);
		return ;
	ft_assert(st_ex->pid != -1);
	while (ST_WAIT)
	{
		if (waitpid(st_ex->pid, &status, 0) != -1 )
			break ;
	}
	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)
		st_ex->c_c = true;
	st_ex->status = WEXITSTATUS(status) + WIFSIGNALED(status) * 128 + WIFSIGNALED(status) * WTERMSIG(status);
}

static inline void forward_exit_status(t_status res)
{
	ft_assert(res.status != -1);

	if (res.c_c)
	{
		default_signal_handlers();
		kill(0, SIGINT);
		while (true)
			;
	}
	exit(res.status);
}

static inline void set_cmd_status(t_status *last_cmd_status_res, t_status res, char **last_cmd_status_s)
{
	*last_cmd_status_res = res;
	if (*last_cmd_status_s)
		free(*last_cmd_status_s);
	*last_cmd_status_s = ft_itoa(res.status);
}

static inline char *exe_path(char **path_dirs, char *exe_name)
{
	t_string	temp;
	int			i;

	if (strchr(exe_name, '/'))
		return (strdup(exe_name));
	if (strlen(exe_name) == 0)
		return (0);
	vec_init(&temp);
	i = -1;
	while (path_dirs && path_dirs[++i])
	{
		vec_clear(&temp);
		vec_push(&temp, path_dirs[i]);
		vec_push(&temp, '/');
		vec_push(&temp, exe_name);
		if (access(temp.buff, F_OK) == 0)
			break ;
		else if (path_dirs[i + 1] == 0)
			return (free(temp.buff), NULL);	
	}
	if (temp.len > 0)
		return (temp.buff);
	return (free(temp.buff, NULL));
}

static inline bool	check_is_a_dir(char *path, bool *enoent)
{
	t_stat	info;

	*enoent = false;
	if (stat(path, &info) == -1)
	{
		if (errno == ENOENT)
		{
			*enoent = true;
			return (false);
		}
		critical_error_errno_context(path);
	}
	return(S_ISDIR(info.st_mode));
}

//static int	cmd_not_found(char *cmd_name)
//{
//
//}
//
//static int no_such_file_or_dir(char *cmd_name, char *path_of_exe)
//{
//	
//}
//
//static inline int find_cmd_path(char *cmd_name, char **path_of_exe)
//{
//	
//}
#endif