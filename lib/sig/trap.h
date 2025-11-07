/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   trap.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 21:16:02 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 00:04:51 by dlesieur         ###   ########.fr       */
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
	SIG_ASYNCSIG = 0X80,			// The signal is ignoed because its'async cmd
	SIG_BLOCK = 0x160,
	SIG_SET_MASK = 0X320
}	t_sigs;

# define sig_handler				void
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

typedef unsigned int	t_sigset;
typedef void			(*t_sig_handler)(int);

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
	sig_handler		*original_signals[NSIG];
	char			*trap_list[BASH_NSIG];
	int				pending_traps[NSIG];
	int				running_traps;
	int				trap_return_context;
	int				trap_saved_exit_value;
	int				wait_signal_received;
	int				trapped_signal_received;
	int				suppress_debug_trap_verbose;
}	g_sig = {0};

typedef struct s_glob_sig	t_glob_sig;

static inline void	get_original_signal(int sig)
{
	if (sig <= 0 || sig >= NSIG)
		return ;
}

static inline void	setup_original_sig(int sig)
{
	if (sig > 0 && sig < NSIG
		&& g_sig.original_signals[sig] == IMPOSSIBLE_TRAP_HANDLER)
		get_original_signal(sig);
}

#endif