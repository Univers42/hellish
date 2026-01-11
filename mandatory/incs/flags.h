/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   flags.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 19:24:13 by dlesieur          #+#    #+#             */
/*   Updated: 2025/10/29 19:50:40 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FLAGS_H
# define FLAGS_H

# define FLAG_ON '-'
# define FLAG_OFF '+'
# define FLAG_ERROR -1
# define FLAG_UNKNOWN 0

typedef struct flag_alist
{
	char	name;
	int		*value;
}	t_flag_alist;

typedef struct s_shflags
{
	int	mark_modified_vars;
	int	errexit_flag;
	int	exi_immediately_on_error;
	int	disallow_filename_globbing;
	int	place_keywords_in_env;
	int	read_but_dont_execute;
	int	just_one_cmd;
	int	unbound_var_is_error;
	int	echo_input_at_read;
	int	verbose_flag;
	int	echo_cmd_at_execute;
	int	noclobber;
	int	hashing_enabled;
	int	forced_interactive;
	int	privileged_mode;
	int	jobs_m_flag;
	int	asynchronous_notification;
	int	interactive_comments;
	int	no_symbolic_links;
	int	function_trace_mode;
	int	error_trace_mode;
	int	pipefail_opt;
	// -c, -s invocation options -- not really flags,but they show up in $- 
	int	want_pending_command;
	int	read_from_stdin;
	int	lexical_scoping;
	int	brace_expansion;
	int	history_expansion;
	int	histexp_flag;
	int	restricted;
	int	restricted_shell;
}	t_shflags;

/**
 * Singleton accessor for all shell flags.
 * Returns a pointer to a static instance of the flags structure.
 * The implementation (in a .c file) will initialize this struct
 */
t_shflags			*access_flags(void);

/**
 * Public functions prototypes
 * these functions will use internally access_flags() to access flag data.
 */
extern int			*find_flag(int, int);
extern int			change_flag(int, int);
extern char			*which_set_flags(void);
extern void			reset_shell_flags(void);
extern char			*get_cur_flags(void);
extern void			set_cur_flags(const char *);
extern void			initialize_flags(void);

static inline int	chflagc(int flag, int on_or_off)
{
	return (change_flag(flag, on_or_off));
}
#endif/*FLAGS_h*/
