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

/* Job control public types and API.
   A "job" is a background pipeline identified by a process group (pgid).
   The table is a flat array of JOB_MAX slots; a zero pgid means free.
   Job IDs are monotonically increasing and never recycled within a session,
   which matches bash behaviour and prevents the "stale $!" bug. */

#ifndef JOB_CONTROL_H
# define JOB_CONTROL_H

# include "libft.h"
# include <sys/types.h>
# include <signal.h>

/* Maximum simultaneous background jobs. 256 is ample for any interactive
   session; going higher just wastes stack space (the table is embedded). */
# define JOB_MAX 256

/* Lifecycle states for a background job. RUNNING -> STOPPED (SIGSTOP / ^Z);
   RUNNING -> DONE (normal exit); RUNNING -> KILLED (signal death, kept distinct
   from DONE for display). Keep comments OFF the enumerator lines below --
   norminette mis-parses an inline comment on an enum value. */
typedef enum e_job_status
{
	JOB_RUNNING,
	JOB_STOPPED,
	JOB_DONE,
	JOB_KILLED
}	t_job_status;

/* Metadata for one background job (one pipeline). */
typedef struct s_job
{
	int				id; /* shell-level job number ([N] in `jobs`) */
	pid_t			pgid; /* process group; 0 = slot is free */
	t_job_status	status; /* current lifecycle state */
	int				exit_code; /* last recorded exit code (0 before exit) */
	char			*cmd; /* command text for display (owned, heap) */
	bool			notified; /* true once the "Done" line was printed */
	bool			bg; /* true if started with & (not fg'd yet) */
}	t_job;

/* The job table embedded in t_shell.  current/previous are job IDs
   (not slot indices) used to resolve %%, %+, %-  jobspecs. */
typedef struct s_job_table
{
	t_job	jobs[JOB_MAX]; /* flat array of job slots */
	int		count; /* number of occupied slots */
	int		next_id; /* next job ID to assign (monotonic) */
	int		current; /* ID of the most-recently-backgrounded job */
	int		previous; /* ID of the job before current */
}	t_job_table;

struct	s_shell;

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
