/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   trap.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 21:16:02 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 21:33:30 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TRAP_H
# define TRAP_H
#include <stdio.h>
#include <errno.h>
#include <signal.h>

typedef enum e_sigs
{
	SIG_INHERITED = 0X0,        //value inherited from parent.
	SIG_TRAPPED = 0X1,          // Currently trapped
	SIG_HARD_IGNORE = 0X2,		// Signal was ignored on shell entry
	SIG_SPECIAL = 0X4,			// Treat this signal  specially
	SIG_NO_TRAP = 0X8,			// signal cannot be trapped
	SIG_INPROGRESS = 0X10,		// signal hndler currently executing
	SIG_CHANGED = 0X20,			// trap value changed in trap handler
	SIG_IGNORED = 0X40,			// The signal is currently beign ignored
	SIG_ASYNCSIG = 0X80			// The signal is ignoed because its' in an async cmd
}   t_sigs;

# define NSIG 64	// system signall + 3 customes one
# define DSIG_SIGPREFIX 0X01
# define DSIG_NOCASE	0x02
# define NO_SIG			-1
# define DEBUG_TRAP		NSIG
# define ERROR_TRAP		65
# define RETURN_TRAP	66
# define EXIT_TRAP		0
# define DEFAULT_SIG	SIG_DFL
# define IGNORE_SIG		SIG_IGN
# define BASH_NSIG		67

typedef void (*sig_handler)(int);

typedef struct s_glob_sig
{
	int				sigmodes[BASH_NSIG];
	volatile int	from_return_trap;
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
}	t_glob_sig;

void	get_original_signal(int sig)
{
	t_glob_sig	*g_sig;

	access_sig_datas()
	if (sig <= 0 || sig >= NSIG)
		return ;
	
}
#endif