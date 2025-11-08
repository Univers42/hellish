/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   trap.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 21:16:02 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 07:07:42 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TRAP_H
# define TRAP_H
# include <stdio.h>
# include <errno.h>
# include <signal.h>

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
	SIG_ASYNCSIG = 0X80,		// The signal is ignoed because its'async cmd
	SIG_BLOCK = 0x160,
	SIG_SET_MASK = 0X320
}	t_sigs;

# define IMPOSSIBLE_TRAP_HANDLER	-1
# define NSIG						64	// system signall + 3 customes one
# define DSIG_SIGPREFIX				0X01
# define DSIG_NOCASE				0x02
# define NO_SIG						-1
# define DEBUG_TRAP					NSIG
# define ERROR_TRAP					65
# define RETURN_TRAP				66
# define EXIT_TRAP					0
# define DEFAULT_SIG				SIG_DFL
# define IGNORE_SIG					SIG_IGN
# define BASH_NSIG					67

typedef unsigned int		t_sigset;
typedef void				(*t_sig_handler)(int, void *);
typedef struct sigaction	t_sigaction;
typedef struct s_glob_sig	t_glob_sig;
//the global turtle
struct s_glob_sig
{
	int				sigmodes[BASH_NSIG];
	volatile int	from_return_trap;
	volatile int	sigwinch_received;
	volatile int	sigterm_received;
	int				interrupt_immediately;
	int				terminate_immediately;
	int				builtin_catch_sigpipe;
	int				waiting_for_child;
	void			*original_signals[NSIG];
	char			*trap_list[BASH_NSIG];
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
}	g_sig = {0};

static inline void	get_original_signal(int sig)
{
	if (sig <= 0 || sig >= NSIG)
		return ;
}

static inline void	setup_original_sig(int sig)
{
	if (sig > 0 && sig < NSIG
		&& g_sig.original_signals[sig] == (void*) IMPOSSIBLE_TRAP_HANDLER)
		get_original_signal(sig);
}

static inline int spec_trap(int s)
{
	return ((s == EXIT_TRAP) || (s == DEBUG_TRAP)
			|| s == ERROR_TRAP || s == RETURN_TRAP);
}

static inline int recursive_sig(int s)
{
	return (spec_trap(s) == 0);
}

void			change_signal(int sig, char *value);
int				any_signals_trapped(void);
void			check_signals_and_traps(void);
void			check_signals(void);
void			clear_pending_traps(void);
int				decode_signal(const char *string, int flags);
int				first_pending_trap(void);
void			init_chld_signal(void);
void			init_int_signal(void);
void			free_trap_command(int sig);
void			free_trap_string(int sig);
void			free_trap_strings(void);
void			get_all_original_signals(void);
void			get_orig_sig(int sig);
void			get_original_signal(int sig);
void			is_term_signal(void);
void			check_termsig(void);
int				last_signal(void);
void			reset_sigterm(void);
void			ignore_signal(int sig);
void			init_quit_signal(void);
void			initialize_traps(void);
int				maybe_call_trap_handler(int sig);
int				next_pending_trap(int start);
unsigned int	ft_sigmask(int x);
void			ft_sigemptyset(t_sigset *set);
int				ft_sigismember(const t_sigset *set, int sig);
int				sigblock(int mask);
int				ft_sigismember(const t_sigset *set, int sig);
void			ft_sigfillset(t_sigset *set);
void			ft_sigaddset(t_sigset *set, int sig);
void			ft_sigdelset(t_sigset *set, int sig);
void			check_sigterm(void);
void			zreset_if_needed(void);
void			reinit_trap(int sig);
void			reset_signal_handlers(void);
void			reset_signal(int sig);
void			restore_bash_trapsig(char *oldval);
void			restore_default_signal(int sig);
void			restore_original_signals(void);
void			restore_signal(int sig);
char			*save_bash_trapsig(void);
void			set_bash_trapsig(int sig);
void			set_bash_trapsig(int sig);
void			set_debug_trap(const char *command);
void			set_error_trap(const char *command);
t_sig_handler	*set_orig_sig(int sig, void *handler);
t_sig_handler	*set_original_signal(int sig, void *handler);
void			set_return_trap(const char *command);
void			*set_sigint_handler(void);
void			set_signal_async_ignored(int sig);
void			set_signal_hard_ignored(int sig);
void			set_signal_ignored(int sig);
void			set_signal(int sig, const char *string);
void			set_trap_state(int sig);
void			set_signal_async_ignored(int sig);
void			set_signal_hard_ignored(int sig);
void			set_signal_ignored(int sig);
void			set_signal(int sig, const char *string);
void			set_trap_state(int sig);
t_sig_handler	*set_signal_handler(int sig, void *handler);
int				ft_sigpause(int mask);
t_sig_handler	*set_signal_handler(int sig, void *handler);
int				ft_sigprocmask(int operation, int *newset, int *oldset);
int				ft_sigsuspend(const t_sigset *set);
int				signal_in_progress(int sig);
int				signal_is_async_ignored(int sig);
int				signal_is_hard_ignored(int sig);
int				signal_is_ignored(int sig);
int				signal_is_pending(int sig);
int				signal_is_special(int sig);
int				signal_is_trapped(int sig);
char			*signal_name(int sig);
void			trap_if_untrapped(int sig, const char *command);
void			*trap_to_sighandler(int sig);
void			uw_maybe_set_return_trap(void *command);
void			uw_maybe_set_debug_trap(void *command);
void			uw_maybe_set_error_trap(void *command);
void			uw_set_error_trap(void *command);

#endif
