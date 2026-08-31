/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_zsh3.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 04:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 04:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"

/* %F{#rrggbb}: truecolour, the way zsh 5.9 emits it (38;2;r;g;b). */
bool	zsh_color_hex(char *buf, size_t n, const char *s, int base)
{
	long	v;
	int		i;

	if (*s != '#')
		return (false);
	v = 0;
	i = 1;
	while (i <= 6 && ft_isxdigit(s[i]))
	{
		v = v * 16 + (ft_strchr("0123456789abcdef",
					ft_tolower(s[i])) - "0123456789abcdef");
		i++;
	}
	if (i != 7)
		return (false);
	snprintf(buf, n, "\\[\\e[%d;2;%ld;%ld;%ldm\\]", base,
		(v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff);
	return (true);
}

/* %l and %y: the terminal device. Measured on the oracle: no tty at all
   answers "()"; otherwise /dev/ is stripped, and %l also drops a leading
   "tty" so /dev/tty1 reads "1" while /dev/pts/0 stays "pts/0". */
static void	zsh_tty(t_string *out, char c)
{
	char	*name;

	name = ttyname(STDIN_FILENO);
	if (!name)
		return ((void)vec_push_str(out, "()"));
	if (ft_strncmp(name, "/dev/", 5) == 0)
		name += 5;
	if (c == 'l' && ft_strncmp(name, "tty", 3) == 0 && name[3])
		name += 3;
	zsh_inject(out, name);
}

/* %v / %Nv: element n of psvar, 1-based like zsh, read straight from the
   stored array rather than written back through ${psvar[n]} -- the
   subscript's base would then depend on which dialect happens to be
   armed at render time, and a prompt must not change meaning with the
   mode. The dialect's 1-based subscripts sit over 0-based storage, hence
   n - 1. A scalar psvar answers for element 1, an absent one for none. */
static void	zsh_psvar(t_shell *state, t_string *out, int n)
{
	char	*val;
	char	*elem;

	val = env_expand(state, "psvar");
	if (!val || !*val)
		return ;
	if (!arr_is(val))
	{
		if (n == 1)
			zsh_inject(out, val);
		return ;
	}
	elem = arr_get_idx(val, n - 1);
	if (elem)
		zsh_inject(out, elem);
	xfree(elem);
}

/* The computed identity escapes.
   %# is `%` for a mortal and `#` for root -- NOT the bash `$`, and the
   oracle is what caught the difference. %e is the evaluation depth,
   which is the call-frame stack's height here. */
bool	zsh_ident(t_shell *state, t_string *out, t_zesc *z, char c)
{
	char	buf[32];
	int		n;

	if (c == '#')
	{
		if (geteuid() == 0)
			return (vec_push_char(out, '#'), true);
		return (vec_push_char(out, '%'), true);
	}
	if (c == 'l' || c == 'y')
		return (zsh_tty(out, c), true);
	if (c == 'e')
		return (snprintf(buf, sizeof(buf), "%d",
				(int)state->call_frames.len), vec_push_str(out, buf), true);
	if (c != 'v')
		return (false);
	n = 1;
	if (z->has_n)
		n = z->n;
	zsh_psvar(state, out, n);
	return (true);
}
