/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor_types.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:04:35 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:39:37 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXECUTOR_TYPES_H
# define EXECUTOR_TYPES_H

# include <stdbool.h>

typedef enum s_res_t
{
	RES_OK,
	RES_ERR,
	RES_GETMOREINPUT,
	RES_INIT,
}	t_result_type;

/**
 * status = exit status for the current command or executor
 * pid = process id (child pid) associated with the execution
 * ctrl_c = true if the execution was interrupted by a SIGINT
 */
typedef struct s_execution_state
{
	int		status;
	int		pid;
	bool	ctrl_c;
}	t_execution_state;

static inline t_execution_state	create_exec_state(int status,
													bool ctrl_c)
{
	return ((t_execution_state)
		{
			.status = status,
			.pid = 0,
			.ctrl_c = ctrl_c
		});
}

static inline t_execution_state	create_exec_state_pid(int status,
											int pid,
											bool ctrl_c)
{
	return ((t_execution_state)
		{
			.status = status,
			.pid = pid,
			.ctrl_c = ctrl_c
		});
}

#endif
