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
# include <sys/stat.h>

typedef struct s_exe_cmd
{
	t_env		pre_assigns;
	t_strings	argv;
	char		*fname;
}	t_exe_cmd;

typedef struct s_exe_node
{
	int			infd;
	int			outfd;
	int			next_infd;
	t_ast_node	*node;
	t_ints		redirs;
	bool		modify_parent_context;
}	t_exe_node;

typedef struct s_exe_st
{
	int		status;
	int		pid;
	bool	c_c;
}	t_exe_st;

/* type aliases */
typedef t_vec		t_exe_res;
typedef struct stat	t_stat;

/* ... stub helpers only; implementations in executor.c ... */

#endif