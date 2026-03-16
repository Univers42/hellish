/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_control.h                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef JOB_CONTROL_H
# define JOB_CONTROL_H

# include "libft.h"
# include <sys/types.h>
# include <signal.h>

# define JOB_MAX 256

typedef enum e_job_status
{
	JOB_RUNNING,
	JOB_STOPPED,
	JOB_DONE,
	JOB_KILLED
}	t_job_status;

typedef struct s_job
{
	int			id;
	pid_t		pgid;
	t_job_status	status;
	int			exit_code;
	char		*cmd;
	bool		notified;
	bool		bg;
}	t_job;

typedef struct s_job_table
{
	t_job	jobs[JOB_MAX];
	int		count;
	int		next_id;
	int		current;
	int		previous;
}	t_job_table;

struct s_shell;

void	job_table_init(t_job_table *jt);
t_job	*job_add(t_job_table *jt, pid_t pgid, const char *cmd, bool bg);
t_job	*job_find_id(t_job_table *jt, int id);
t_job	*job_find_pgid(t_job_table *jt, pid_t pgid);
void	job_remove(t_job_table *jt, int id);
void	job_update_status(t_job_table *jt);
void	job_notify(struct s_shell *state);
t_job	*job_by_spec(t_job_table *jt, const char *spec);
void	job_set_current(t_job_table *jt, int id);
void	job_print(t_job *job, int current, int prev, bool show_pid);
void	job_table_free(t_job_table *jt);

#endif
