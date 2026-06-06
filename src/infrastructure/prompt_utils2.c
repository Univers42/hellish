/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 05:14:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Wrap an ANSI escape sequence in the \001...\002 bracket pair that readline
   requires around all zero-width sequences in the prompt string. Without these
   markers readline miscounts the visible width and the cursor drifts when you
   navigate history or use tab completion on a coloured prompt. */
void	vec_push_ansi(t_string *v, const char *seq)
{
	vec_push_char(v, '\001');
	vec_push_str(v, seq);
	vec_push_char(v, '\002');
}

/* Terminal width for right-padding the prompt box to the edge. We check
   $COLUMNS first (set by the terminal or manually) so scripts can override the
   detected value; then TIOCGWINSZ; finally a safe default of 80. */
int	get_cols(void)
{
	char			*env;
	int				c;
	struct winsize	ws;

	env = getenv("COLUMNS");
	if (env)
	{
		c = atoi(env);
		if (c > 0)
			return (c);
	}
	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
		return ((int)ws.ws_col);
	return (80);
}

/* Walk `p` decoding each multibyte character and accumulating display columns.
   Same reset-on-error trick as visible_width_cstr: a bad byte is counted as
   width 1 and the mbstate is cleared so subsequent characters can still decode.
*/
static void	process_measurement(const char *p, mbstate_t *state, int *total)
{
	int		w;
	wchar_t	wc;
	size_t	len;

	while (*p)
	{
		len = mbrtowc(&wc, p, MB_CUR_MAX, state);
		if (len == (size_t) - 1 || len == (size_t) - 2)
		{
			(*total) += 1;
			p++;
			ft_memset(state, 0, sizeof(*state));
			continue ;
		}
		if (len == 0)
			break ;
		w = wcwidth(wc);
		if (w < 0)
			w = 0;
		(*total) += w;
		p += len;
	}
}

/* Terminal display width of a plain string (no \001/\002 markers, unlike
   visible_width_cstr). Used to measure prompt segments that are already
   stripped of escape brackets, e.g. the branch name or cwd component. */
int	measure_width(const char *str)
{
	mbstate_t	state;
	int			total;
	const char	*p = str;

	total = 0;
	ft_memset(&state, 0, sizeof(state));
	process_measurement(p, &state, &total);
	return (total);
}

/* Shorten a long path to fit in `maxlen` bytes by replacing a long leading
   prefix with ".../" while keeping as many trailing path components as fit.
   We walk backwards from the end to find a slash boundary rather than cutting
   mid-component, so the result is always a valid path fragment. */
char	*shorten_path(const char *path, int maxlen)
{
	size_t			plen;
	const char		*p;
	int				kept;
	char			*out;

	plen = ft_strlen(path);
	if ((int)plen <= maxlen)
		return (ft_strdup(path));
	p = path + plen;
	kept = 0;
	while (p > path && kept < maxlen - 4)
	{
		p--;
		if (*p == '/')
			kept = (int)(path + plen - p - 1);
	}
	if (p <= path || kept == 0)
		return (ft_strdup(path + plen - (maxlen - 4)));
	out = ft_strjoin(".../", p + 1);
	return (out);
}
