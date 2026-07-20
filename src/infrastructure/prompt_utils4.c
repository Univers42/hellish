/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils4.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 01:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/07/21 01:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

#define C_CONN2 "\033[38;5;243m"
#define C_DUR   "\033[38;5;179m"
#define C_JOBS  "\033[1m\033[38;5;110m"
#define C_ROOT  "\033[1m\033[38;5;203m"
#define C_USER2 "\033[1m\033[38;5;81m"
#define C_RST2  "\033[0m"

/* Snapshot cells for state-derived prompt segments, in the same style as
   anim_frame()/anim_status(): the readline idle hook re-renders the
   prompt without access to t_shell, so prompt_normal copies what it
   needs into these before rendering. */
long long	*anim_dur_ms(void)
{
	static long long	dur;

	return (&dur);
}

int	*anim_jobs(void)
{
	static int	jobs;

	return (&jobs);
}

/* Root gets a red username — the classic "you are dangerous" cue. */
const char	*user_color(void)
{
	if (getuid() == 0)
		return (C_ROOT);
	return (C_USER2);
}

/* "3.4s" under a minute, "2m07s" under an hour, "1h12m" beyond. */
static void	fmt_duration(long long ms, char *buf, size_t n)
{
	long long	s;

	s = ms / 1000;
	if (s < 60)
		snprintf(buf, n, "%lld.%llds", s, (ms % 1000) / 100);
	else if (s < 3600)
		snprintf(buf, n, "%lldm%02llds", s / 60, s % 60);
	else
		snprintf(buf, n, "%lldh%02lldm", s / 3600, (s % 3600) / 60);
}

/* Optional top-row segments after the venv: a background-jobs badge
   (⚙N) whenever jobs exist, and "took N.Ns" once the last foreground
   command crossed 2 seconds — long enough to skip the noise of instant
   commands, short enough to catch anything you actually waited on.
   Both update p->vis_w so the right-aligned clock stays flush, and each
   is DROPPED when the row lacks room for it plus the minimum fill and
   the clock (the 22/28 thresholds) — a narrow terminal loses the
   accessories, never the box alignment. */
void	render_extras(t_string *ret, t_prompt *p)
{
	char	buf[24];

	if (*anim_jobs() > 0 && p->cols - p->vis_w > 22)
	{
		snprintf(buf, sizeof(buf), "\xe2\x9a\x99%d", *anim_jobs());
		vec_push_str(ret, " ");
		vec_push_ansi(ret, C_JOBS);
		vec_push_str(ret, buf);
		vec_push_ansi(ret, C_RST2);
		p->vis_w += measure_width(buf) + 1;
	}
	if (*anim_dur_ms() >= 2000 && p->cols - p->vis_w > 28)
	{
		fmt_duration(*anim_dur_ms(), buf, sizeof(buf));
		vec_push_str(ret, " ");
		vec_push_ansi(ret, C_CONN2);
		vec_push_str(ret, "took ");
		vec_push_ansi(ret, C_DUR);
		vec_push_str(ret, buf);
		vec_push_ansi(ret, C_RST2);
		p->vis_w += (int)ft_strlen(buf) + 6;
	}
}
