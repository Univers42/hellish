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

/* Core execution result types: kept in a separate header so they can be
   included without pulling in the full t_shell definition (avoids
   circular dependency between executor.h and shell.h). */

#ifndef EXECUTOR_TYPES_H
# define EXECUTOR_TYPES_H

# include <stdbool.h>

/* Top-level parse/execute result code.  RES_GETMOREINPUT is the key one:
   it signals that the input ended mid-compound-command (e.g. `if` with no
   `fi` yet) and the REPL should display PS2 and read another line. */
typedef enum s_res_t
{
	RES_OK,           /* execution succeeded */
	RES_ERR,          /* execution failed (hard error) */
	RES_GETMOREINPUT, /* input is syntactically incomplete, need more */
	RES_INIT,         /* initial / uninitialised state */
}	t_result_type;

/* Return value from every execute_* function.  All three fields flow up
   the call stack so the REPL can set $?, $!, and re-display the prompt
   correctly after a Ctrl-C even inside a subshell. */
typedef struct s_execution_state
{
	int		status;  /* exit status (what becomes $?) */
	int		pid;     /* child PID for background jobs (what becomes $!) */
	bool	ctrl_c;  /* true if SIGINT was received during this command */
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
