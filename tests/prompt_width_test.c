/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_width_test.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 23:55:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 23:55:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* visible_width_cstr -- the prompt width model, tested directly.
**
** A UNIT test, and deliberately so. The width is what the line editor uses
** to place the cursor; it is not printed anywhere and no shell-level command
** reveals it, so the only observable at the shell level is where a line wraps
** on a terminal. A pty case that reads that back was tried first and was too
** timing-dependent to gate on -- and a flaky test is worse than none, because
** it teaches you to ignore a red run.
**
** Linking the object file directly costs one ft_memset shim and gives an
** exact answer every time. tests/prompt_width_test.sh builds and runs it.
**
** THE BUG THIS EXISTS FOR (#72): skip_ansi_escape handled CSI (ESC [) but
** not OSC (ESC ]), so an unguarded window-title sequence had its whole
** payload counted as visible columns. It is the one escape whose payload is
** readable text, so a 30-character title measured 30 columns too wide and
** the editor wrapped that far early, overwriting the line. Against the
** object built before the fix, five of these ten cases fail.
*/

#include <stdio.h>
#include <string.h>
#include <locale.h>

int	visible_width_cstr(const char *s);

/* The object under test is compiled against libft; ft_memset is the only
   symbol it needs and memset is the same function. */
void	*ft_memset(void *b, int c, unsigned long n)
{
	return (memset(b, c, n));
}

static struct s_case
{
	const char	*s;
	int			want;
	const char	*why;
}	g_cases[] = {
{"abc", 3, "plain ascii"},
{"", 0, "empty"},
{"\001\033[32m\002ab", 2, "guarded SGR is zero-width"},
{"\033[32mab", 2, "unguarded SGR is zero-width too"},
{"\001\033]0;LONG-TITLE-TEXT\a\002ab", 2, "guarded OSC"},
{"\033]0;LONG-TITLE-TEXT\aab", 2, "UNGUARDED OSC -- issue #72"},
{"\033]0;T\033\\ab", 2, "OSC ended by ST, not BEL"},
{"\033]0;unterminated", 0, "unterminated OSC eats the rest"},
{"\033]0;A\aX\033]0;B\aY", 2, "two OSCs in one prompt"},
{"ab\033]0;T\a", 2, "OSC at the end"},
{"\001\033[1m\002\033]0;T\a\033[32mx\001\033[0m\002", 1,
	"all three kinds at once"},
{NULL, 0, NULL}
};

int	main(void)
{
	int	bad;
	int	got;
	int	i;

	setlocale(LC_ALL, "");
	bad = 0;
	i = -1;
	while (g_cases[++i].s)
	{
		got = visible_width_cstr(g_cases[i].s);
		if (got != g_cases[i].want)
		{
			bad++;
			printf("FAIL width: %s  (got %d, want %d)\n",
				g_cases[i].why, got, g_cases[i].want);
		}
		else
			printf("ok   width: %s\n", g_cases[i].why);
	}
	if (bad)
		printf("\n%d failed\n", bad);
	else
		printf("\nall passed\n");
	return (bad != 0);
}
