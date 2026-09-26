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
/* get_git_info's dirty word is a bitmask, and a plain `if (dirty)` still
   reads it: GIT_DIRTY means tracked changes of any kind (the \g star),
   the other two say WHERE -- what zsh's vcs_info renders as %c and %u. */
# define GIT_DIRTY 1
# define GIT_STAGED 2
# define GIT_UNSTAGED 4
# define GIT_UNTRACKED 8
# define GIT_UNMERGED 16

/* The prompt's git scan beyond the dirty bits, for vcs_info's HELLISH_GIT_*
   variables (builtin_zsh_vcs2.c): ahead/behind/stash of the repository
   rooted at `root`, 0 when the cached answer is for another one. */
typedef struct s_gitcounts
{
	int	ahead;
	int	behind;
	int	stash;
}	t_gitcounts;

t_gitcounts	git_counts(const char *root);
int			*git_untracked_cell(void);
int			*git_detached_cell(void);
void		git_redir_touch(const char *path);
bool		git_dir_for(const char *root, char *out, size_t cap);

void		get_git_info(char **branch, int *dirty);
const char	*git_repo_root(void);
/* Invalidate the prompt's cached git-dirty answer: called where the working
   tree can actually change (an external command, a write redirection), not
   once per executed tree -- see the comment on the definition. */
void		git_tree_touched(void);

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
	/* --- readline, initialised once in the parent (rl_preinit.c) --- */
	bool		rl_ready; /* rl_initialize() has run in this process */
	bool		use_fork; /* read in a forked child (HELLISH_RL_FORK=1) */
	bool		ps1_read; /* the prompt being read is PS1's, not PS2's */
	char		*shown_prompt; /* the prompt on screen, while reading */
	int			mode_applied; /* edit_mode readline's keymap is set to */
	size_t		binds_done; /* zle bindings already installed */
	size_t		bind_lines_done; /* `bind` requests already applied */
	/* --- RPROMPT: rendered here, painted by the editor (rl_rprompt.c) --- */
	t_string	rp_txt; /* rendered right prompt, width markers stripped */
	int			rp_w; /* its terminal width; 0 = nothing to paint */
	int			rp_prompt_w; /* width of the row readline was handed */
	bool		rp_painted; /* the clock is on screen right now */
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
int			get_more_input_readline(t_shell *state, char *prompt);
void		update_ctx(t_shell *state);
int			get_more_input_notty(t_shell *state);

int			rl_read_fork(t_shell *state, char *prompt);
int			rl_read_inproc(t_shell *state, char *prompt);
char		*rl_editor_enter(t_shell *state, char *prompt);
void		rl_editor_exit(t_shell *state);
t_string	prompt_normal(t_shell *state);
char		*prompt_expand_p(t_shell *state, const char *fmt);
char		*prompt_more_input(t_shell *state, struct s_parser *parser);
char		*prompt_ps2(t_shell *state, const char *fallback);
void		buff_readline_init(t_rl *ret);
bool		rl_eof_exit_ok(t_shell *state);
void		update_ctx(t_shell *state);

#endif