/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_vcs2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "prompt.h"

/* vcs_info's second output: the repository state as plain variables.
**
** vcs_info_msg_0_ is one formatted string, which is what zsh prompts want.
** A prompt framework that builds its own segment -- colour by state, an
** ahead arrow, a stash count -- had to fork git for each fact, and one did
** seven times per prompt. These come from the same background scan the
** \g escape reads, so they cost the prompt no process at all:
**
**     HELLISH_GIT_BRANCH     branch, or the 7-hex commit when detached
**     HELLISH_GIT_DETACHED   1 when HEAD is detached
**     HELLISH_GIT_ROOT       work tree root
**     HELLISH_GIT_DIR        git dir (a linked worktree's lives under
**                            the main repo's .git/worktrees/)
**     HELLISH_GIT_STAGED     1 when the index differs from HEAD
**     HELLISH_GIT_UNSTAGED   1 when the work tree differs from the index
**     HELLISH_GIT_UNTRACKED  1 when untracked files exist -- scanned only
**                            while HELLISH_VCS_UNTRACKED is true, since
**                            the untracked walk is the costly part
**     HELLISH_GIT_UNMERGED   1 during a conflicted merge
**     HELLISH_GIT_AHEAD      commits ahead of the upstream
**     HELLISH_GIT_BEHIND     commits behind it
**     HELLISH_GIT_STASH      stash entries
**
** Every one is set on every call -- empty or 0 outside a repository -- so
** `${HELLISH_GIT_AHEAD+set}` also tells a script this shell has them. The
** scan is asynchronous: the flags describe the last completed scan, which
** is at most one prompt behind (prompt_git3.c). */

static void	vcs_set(t_shell *state, const char *name, const char *val)
{
	env_set(&state->env, env_create(ft_strdup((char *)name),
			ft_strdup((char *)val), false));
}

static void	vcs_setn(t_shell *state, const char *name, int n)
{
	char	*s;

	s = ft_itoa(n);
	if (!s)
		return ;
	vcs_set(state, name, s);
	xfree(s);
}

static void	vcs_bits(t_shell *state, int bits)
{
	vcs_setn(state, "HELLISH_GIT_STAGED", (bits & GIT_STAGED) != 0);
	vcs_setn(state, "HELLISH_GIT_UNSTAGED", (bits & GIT_UNSTAGED) != 0);
	vcs_setn(state, "HELLISH_GIT_UNTRACKED", (bits & GIT_UNTRACKED) != 0);
	vcs_setn(state, "HELLISH_GIT_UNMERGED", (bits & GIT_UNMERGED) != 0);
}

/* Publish what vcs_info just read. `branch` is NULL outside a repository
   (or when HEAD cannot be read), and every variable then reads as empty
   or 0. */
void	vcs_publish(t_shell *state, const char *branch, const char *root,
			int bits)
{
	char		gd[PATH_MAX];
	t_gitcounts	n;

	gd[0] = '\0';
	if (!branch || !root || !git_dir_for(root, gd, sizeof(gd)))
	{
		branch = NULL;
		root = NULL;
		bits = 0;
	}
	n = git_counts(root);
	vcs_setn(state, "HELLISH_GIT_DETACHED", root && *git_detached_cell());
	if (!root)
	{
		branch = "";
		root = "";
	}
	vcs_set(state, "HELLISH_GIT_BRANCH", branch);
	vcs_set(state, "HELLISH_GIT_ROOT", root);
	vcs_set(state, "HELLISH_GIT_DIR", gd);
	vcs_bits(state, bits);
	vcs_setn(state, "HELLISH_GIT_AHEAD", n.ahead);
	vcs_setn(state, "HELLISH_GIT_BEHIND", n.behind);
	vcs_setn(state, "HELLISH_GIT_STASH", n.stash);
}
