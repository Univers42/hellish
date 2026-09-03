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
   A job ID is (highest live ID) + 1, so a number is released as soon as its
   job leaves the table and an idle shell is back at [1] -- see job_next_id
   in src/job_control/job_id.c for why that is bash's rule. */

#ifndef JOB_CONTROL_H
# define JOB_CONTROL_H

# include "libft.h"
# include <sys/types.h>
# include <signal.h>

/* Maximum simultaneous background jobs. 256 is ample for any interactive
   session; going higher just wastes stack space (the table is embedded). */
# define JOB_MAX 256

/* Lifecycle states for a background job. RUNNING -> STOPPED (SIGSTOP / ^Z);
   RUNNING -> DONE (normal exit); RUNNING -> KILLED (signal death, kept
   distinct from DONE because bash names the SIGNAL where it would otherwise
   say "Done" -- see job_status_desc). Comments stay OFF the enumerators --
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
	int				term_sig; /* signal that killed it, 0 if it exited */
	int				raw_status; /* the waitpid() word, kept for `wait` */
	bool			core_dumped; /* that signal also dumped core */
}	t_job;

/* The job table embedded in t_shell.  current/previous are job IDs
   (not slot indices) used to resolve %%, %+, %-  jobspecs. */
typedef struct s_job_table
{
	t_job	jobs[JOB_MAX]; /* flat array of job slots */
	int		count; /* number of occupied slots */
	int		current; /* ID of the most-recently-backgrounded job */
	int		previous; /* ID of the job before current */
}	t_job_table;

struct	s_shell;

void	job_table_init(t_job_table *jt);
t_job	*job_add(t_job_table *jt, pid_t pgid, const char *cmd, bool bg);
int		job_next_id(t_job_table *jt);
int		job_highest_id(t_job_table *jt, int except);
int		job_next_after(t_job_table *jt, int after);
void	job_reelect(t_job_table *jt, int gone);
t_job	*job_find_id(t_job_table *jt, int id);
t_job	*job_find_pgid(t_job_table *jt, pid_t pgid);
void	job_remove(t_job_table *jt, int id);
void	job_purge_done(t_job_table *jt);
void	job_mark_reported(t_job_table *jt, pid_t pid);
void	job_purge_reported(t_job_table *jt);
void	job_mark_reported_except(t_job_table *jt, pid_t keep);
t_job	*job_first_unfinished(t_job_table *jt);
int		job_live_id(t_job_table *jt, int not_this);
void	job_record_exit(t_job *job, int status);
bool	job_finished(const t_job *job);
int		job_kill_group(struct s_shell *st, t_job *job, int sig);
void	job_notify_async(struct s_shell *state);
void	job_update_status(struct s_shell *st);
void	job_notify(struct s_shell *state);
t_job	*job_by_spec(t_job_table *jt, const char *spec);
void	job_set_current(t_job_table *jt, int id);

/* True when an interactive shell must NOT leave yet because a stopped job
   is still on the table -- prints "There are stopped jobs." and remembers
   that it warned, so the next attempt goes through. Lives here rather than
   in the builtins' private header because BOTH ways out of a shell have to
   ask it: the `exit` builtin and end-of-input (Ctrl-D). */
bool	exit_stopped_guard(struct s_shell *state);

/* Hang up this shell's background jobs and WAIT for them, so their dying
   output reaches the terminal before the shell hands it back. Interactive
   only; see job_hangup.c. */
void	jobs_hangup_on_exit(struct s_shell *st);
void	job_print(t_job *job, int current, int prev, bool show_pid);
void	job_table_free(t_job_table *jt);
char	*job_status_desc(const t_job *job);
char	*done_with_code(char *buf, size_t size, int code);
char	*job_core_suffix(const t_job *job);

#endif
