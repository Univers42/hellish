/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/12 00:45:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Map the last open compound token to a short human label for the continuation
   prompt. The parser pushes these token types onto parse_stack as it enters
   compound commands; prompt_more_input walks the stack and formats "if while >
   " so the user can see exactly how deep they are in a nested construct. */
static const char	*prompt_label(t_tt curr)
{
	if (curr == TT_BRACE_LEFT)
		return ("subsh");
	if (curr == TT_PIPE)
		return ("pipe");
	if (curr == TT_AND)
		return ("cmdand");
	if (curr == TT_OR)
		return ("cmdor");
	if (curr == TT_IF)
		return ("if");
	if (curr == TT_WHILE)
		return ("while");
	if (curr == TT_UNTIL)
		return ("until");
	if (curr == TT_FOR)
		return ("for");
	return (NULL);
}

/* Build the PS2-style continuation prompt from the parser's open-construct
   stack: "if > ", "if while > ", etc. The last space is replaced by '>' to
   give it the feel of a depth indicator. An empty stack (shouldn't happen
   here, but guarded) just produces "> ". */
t_string	prompt_more_input(t_parser *parser)
{
	t_string	ret;
	t_tt		curr;
	size_t		i;
	const char	*label;

	i = -1;
	vec_init(&ret);
	ret.elem_size = 1;
	while (++i < parser->parse_stack.len)
	{
		curr = *(int *)vec_idx(&parser->parse_stack, i++);
		label = prompt_label(curr);
		if (!label)
			continue ;
		vec_push_str(&ret, (char *)label);
		vec_push_str(&ret, " ");
	}
	if (ret.len > 0)
		((char *)ret.ctx)[ret.len - 1] = '>';
	return (vec_push_str(&ret, " "), ret);
}

/* Build the whole prompt for a given animation frame and last exit status: the
   blinking mascot, the box (user/cwd/branch/venv), the clock, and the arrow.
   Called both by prompt_normal and, with an advancing frame, by the readline
   idle hook -- so the mascot keeps blinking even while a command is typed. */
void	render_prompt(t_string *ret, size_t frame, int status)
{
	t_prompt	p;
	int			mascot_w;

	p.exit_status = status;
	mascot_w = push_mascot(ret, frame, status);
	prompt_user_and_cwd(ret, &p);
	p.vis_w += mascot_w;
	prompt_branch(ret, &p);
	prompt_venv(ret, &p);
	prompt_time_and_pad(ret, &p);
	xfree(p.short_cwd);
	if (p.branch)
		xfree(p.branch);
	if (p.venv)
		xfree(p.venv);
}

/* One-shot micro-benchmark of the render path, enabled by exporting
   HELLISH_PROMPT_BENCH: one untimed warm-up render (so the TTL-throttled
   git-status fork lands outside the measurement, as it does in real use),
   then 1000 timed renders, mean reported on stderr. This is how the
   "prompt renders in N us" claims in bench/ are produced. */
static void	maybe_bench(void)
{
	static int		done;
	struct timespec	ts[2];
	t_string		r;
	long long		i;

	if (done || !getenv("HELLISH_PROMPT_BENCH"))
		return ;
	done = 1;
	i = -1;
	while (++i < 1001)
	{
		if (i == 1)
			clock_gettime(CLOCK_MONOTONIC, &ts[0]);
		vec_init(&r);
		r.elem_size = 1;
		render_prompt(&r, (size_t)i, 0);
		xfree(r.ctx);
	}
	clock_gettime(CLOCK_MONOTONIC, &ts[1]);
	i = (ts[1].tv_sec - ts[0].tv_sec) * 1000000000LL
		+ (ts[1].tv_nsec - ts[0].tv_nsec);
	fprintf(stderr, "[prompt-bench] %lld ns/render\n", i / 1000);
}

/* Build the primary prompt for an interactive read. The frame counter is
   advanced here so each readline call shows the next blink frame, giving the
   mascot a heartbeat feel even when the user types slowly. The status is
   snapshotted before rendering so the arrow colour reflects the command that
   just finished, not a half-updated value. */
t_string	prompt_normal(t_shell *state)
{
	t_string	ret;
	int			status;

	ensure_locale();
	maybe_bench();
	vec_init(&ret);
	ret.elem_size = 1;
	status = state->last_cmd_st_exe.status;
	(*anim_frame())++;
	*anim_status() = status;
	*anim_dur_ms() = state->last_cmd_ms;
	*anim_jobs() = state->job_table.count;
	render_prompt(&ret, *anim_frame(), status);
	return (ret);
}
