/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   trap.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 21:16:02 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:15:21 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TRAP_H
# define TRAP_H
# include <stdio.h>
# include <errno.h>
# include <signal.h>
# include <stdlib.h>
# include <stdint.h>
# include <string.h>

// Standard signal handler type
typedef void (*t_sig_handler)(int);

typedef enum e_sigs
{
	SIG_INHERITED = 0X0,		//value inherited from parent.
	SIG_TRAPPED = 0X1,			// Currently trapped
	SIG_HARD_IGNORE = 0X2,		// Signal was ignored on shell entry
	SIG_SPECIAL = 0X4,			// Treat this signal  specially
	SIG_NO_TRAP = 0X8,			// signal cannot be trapped
	SIG_INPROGRESS = 0X10,		// signal hndler currently executing
	SIG_CHANGED = 0X20,			// trap value changed in trap handler
	SIG_IGNORED = 0X40,			// The signal is currently beign ignored
	SIG_ASYNCSIG = 0X80			// The signal is ignoed because its'async cmd
}	t_sigs;

# define IMPOSSIBLE_TRAP_HANDLER	-1
# define BASH_NSIG					64	// system signals + custom ones
# define DSIG_SIGPREFIX				0X01
# define DSIG_NOCASE				0x02
# define NO_SIG						-1
# define DEBUG_TRAP					BASH_NSIG
# define ERROR_TRAP					65
# define RETURN_TRAP				66
# define EXIT_TRAP					0
# define DEFAULT_SIG				SIG_DFL
# define IGNORE_SIG					SIG_IGN
# define BASH_NSIG_TOTAL			67
# define GETORIGSIG(sig)			get_orig_sig(sig)
# define SETORIGSIG(sig, handler)	set_orig_sig(sig, handler)
# define SPECIAL_TRAP(sig)			spec_trap(sig)

// Block/unblock a single signal using sigprocmask
void	block_signal(int sig, sigset_t *set, sigset_t *oset);
void	unblock_signal(const sigset_t *oset);

typedef unsigned int		t_sigset;

typedef struct sigaction	t_sigaction;
typedef struct s_glob_sig	t_glob_sig;

static inline int spec_trap(int s)
{
	return ((s == EXIT_TRAP) || (s == DEBUG_TRAP)
			|| s == ERROR_TRAP || s == RETURN_TRAP);
}

static inline int recursive_sig(int s)
{
	return (spec_trap(s) == 0);
}

//the global turtle
struct s_glob_sig
{
	int				sigmodes[BASH_NSIG_TOTAL];
	volatile int	from_return_trap;
	volatile int	sigwinch_received;
	volatile int	sigterm_received;
	int				interrupt_immediately;
	int				terminate_immediately;
	int				builtin_catch_sigpipe;
	int				waiting_for_child;
	void			*original_signals[NSIG];
	char			*trap_list[BASH_NSIG_TOTAL];
	int				pending_traps[NSIG];
	int				running_traps;
	int				trap_return_context;
	int				trap_saved_exit_value;
	int				wait_signal_received;
	int				trapped_signal_received;
	int				suppress_debug_trap_verbose;
	int				catch_flag;
	int				function_trace_mode;
	int				error_trace_mode;
	int				reset_signal;
	char			*signal_name[NSIG];
	int				interactive;
}	static g_sig = {0};


void	change_signal(int sig, char *value);
int		any_signals_trapped(void);
void	check_signals(void);
void	clear_pending_traps(void);
int		decode_signal(const char *string, int flags);
int		first_pending_trap(void);
void	init_chld_signal(void);
void	init_int_signal(void);
void	free_trap_command(int sig);
void	free_trap_string(int sig);
void	free_trap_strings(void);
void	get_all_original_signals(void);
void	get_orig_sig(int sig);
void	get_original_signal(int sig);
int		last_signal(void);
void	reset_sigterm(void);
void	ignore_signal(int sig);
void	init_quit_signal(void);
void	initialize_traps(void);
void	initialize_signames(void);
void	get_original_tty_job_signals(void);
void	initialize_terminating_signals(void);
int		maybe_call_trap_handler(int sig);
int		next_pending_trap(int start);
unsigned int ft_sigmask(int x);
void	ft_sigemptyset(t_sigset *set);
int		ft_sigismember(const t_sigset *set, int sig);
int		ft_sigblock(int mask);
void	ft_sigfillset(t_sigset *set);
void	ft_sigaddset(t_sigset *set, int sig);
void	ft_sigdelset(t_sigset *set, int sig);
void	check_sigterm(void);
void	zreset_if_needed(void);
void	zreset(void);
void	reinit_trap(int sig);
void	reset_signal_handlers(void);
void	reset_or_restore_signal_handlers(void (*func)(int));
void	reset_signal(int sig);
void	restore_bash_trapsig(char *oldval);
void	restore_default_signal(int sig);
void	restore_original_signals(void);
void	restore_signal(int sig);
char	*save_bash_trapsig(void);
void	set_bash_trapsig(int sig);
void	set_debug_trap(const char *command);
void	set_error_trap(const char *command);
void	set_orig_sig(int sig, t_sig_handler *handler);
void	set_original_signal(int sig, t_sig_handler *handler);
void	set_return_trap(const char *command);
void	*set_sigint_handler(void);
void	set_signal_async_ignored(int sig);
void	set_signal_hard_ignored(int sig);
void	set_signal_ignored(int sig);
void	set_signal(int sig, const char *string);
void	set_trap_state(int sig);
int		ft_sigpause(int mask);
void	(*set_signal_handler(int sig, void (*handler)(int)))(int);
int		ft_sigprocmask(int operation, int *newset, int *oldset);
int		ft_sigsuspend(const t_sigset *set);
int		signal_in_progress(int sig);
int		signal_is_async_ignored(int sig);
int		signal_is_hard_ignored(int sig);
int		signal_is_ignored(int sig);
int		signal_is_pending(int sig);
int		signal_is_special(int sig);
int		signal_is_trapped(int sig);
char	*signal_name(int sig);
void	trap_if_untrapped(int sig, const char *command);
void	*trap_to_sighandler(int sig);
void	uw_maybe_set_return_trap(void *command);
void	uw_maybe_set_debug_trap(void *command);
void	uw_maybe_set_error_trap(void *command);
void	uw_set_error_trap(void *command);

// Signal handler functions
void	trap_handler(int sig);
void	sigint_sighandler(int sig);
void	termsig_sighandler(int sig);

// External variable management stubs
void	unbind_variable_noref(const char *name);
void	bind_variable(const char *name, const char *value, int flags);
void	bind_var_to_int(const char *name, int value, int flags);
char	*get_string_value(const char *name);
char	*savestring(const char *s);

#endif
