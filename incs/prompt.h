/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:29:55 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:29:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PROMPT_H
# define PROMPT_H

# include "alias.h"
# include <stdbool.h>
# include <stddef.h>

/* forward-declare shell to avoid pulling full shell.h here */
typedef struct s_shell	t_shell;

/* The prompt an unconfigured hellish shows: zsh's own default -- `%m%# `
   renders "hostname% " (measured on the 5.9 oracle with zsh -f) -- plus
   the \U update badge, which is the one piece of information the shell
   must still be able to volunteer (self-spacing: invisible until a newer
   release is pending, honours HELLISH_NO_UPDATE_CHECK). The rich two-row
   theme did NOT go away; it stopped being the default. `PS1='\B'` or the
   `prompt` switcher bring it back. */
# define HELLISH_PS1_DEFAULT "%m\\U%# "

/* The prompt's fork-free cached git reader (prompt_metadata.c): branch is
   heap-owned by the caller, dirty is the TTL-throttled `git status`
   answer. Public because vcs_info (builtin_zsh_prompt.c) reports the
   same repository state the \g escape renders. */
void		get_git_info(char **branch, int *dirty);

// buff_readline.c
typedef struct s_rl
{
	bool		has_line;
	bool		should_update_ctx;
	bool		has_finished;
	int			line;
	t_string	buff;
	size_t		cursor;
	int			edit_mode;
	bool		no_compact;
	int			cycle_line0; /* line number of the cycle's first line */
	bool		line_exact; /* consumer needs one line per call (heredoc) */
	bool		batched; /* this cycle got a multi-line batch delivery */
	bool		eof_refused; /* this turn's EOF was spent on a warning */
	size_t		exact_until; /* serve single lines below this cursor pos */
	bool		tok_line; /* non-tty cycle: $LINENO from token offset */
	const char	*ln_tok; /* first token of the executing command */
	const char	*ln_ptr; /* memoised lineno lookup key (token ptr) */
	int			ln_val; /* memoised line number for ln_ptr */
}	t_rl;

// Forward declaration to avoid circular dependency
struct					s_parser;

int			buff_readline(t_shell *state, t_string *ret, char *prompt);
int			return_batch(t_shell *state, t_string *ret);
bool		try_replay_exact(t_shell *state);
void		begin_cycle(t_shell *state, t_string *ret);
int			nl_count(const char *s, size_t n);
void		buff_readline_update(t_rl *l);
void		buff_readline_reset(t_rl *l);
int			get_more_input_readline(t_rl *l, char *prompt);
void		update_ctx(t_shell *state);
int			get_more_input_notty(t_shell *state);

void		bg_readline(int outfd, char *prompt, int edit_mode,
				struct s_shell *state);
int			attach_input_readline(t_rl *l, int pp[2], int pid);
t_string	prompt_normal(t_shell *state);
t_string	prompt_more_input(t_shell *state, struct s_parser *parser);
void		buff_readline_init(t_rl *ret);
bool		rl_eof_exit_ok(t_shell *state);
void		update_ctx(t_shell *state);

#endif