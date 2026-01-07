/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 17:29:02 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 17:29:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_H
# define UTILS_H

# include "common.h"
# include <stdint.h>
# include <stdio.h>
# include <fcntl.h>

typedef struct s_prng_state
{
	uint32_t	state_array[624];
	int			state_index;
}	t_prng_state;

static inline bool is_space(int c)
{
	return (c == ' ' || c == '\t');
}

static inline bool is_special_char(int c)
{
	const char	*specials = ";$'\"<>|&()\n";

	return (strchr(specials, c) != NULL || is_space(c));
}

/* Initialize pseudo-random generator state.
   Uses MT19937-style seeding to fill the state array deterministically. */
static inline void	prng_init_state(t_prng_state *state, uint32_t seed)
{
	int i;

	if (state == NULL)
		return ;
	state->state_array[0] = seed ? seed : 5489U;
	i = 1;
	while (i < 624)
	{
		state->state_array[i] = (uint32_t)(1812433253UL
			* (state->state_array[i - 1] ^ (state->state_array[i - 1] >> 30))
			+ (uint32_t)i);
		i++;
	}
	state->state_index = 0;
}

/* Generate a 32-bit pseudo-random value.
   To keep implementation compact we use a simple xorshift step cycling
   over the state array (seeded above). Not a full MT19937 extract, but
   deterministic and adequate for pragmatic use in this codebase. */
static inline uint32_t	random_uint32(t_prng_state *state)
{
	uint32_t x;
	int idx;

	if (state == NULL)
		return (0);
	idx = state->state_index;
	x = state->state_array[idx];
	/* xorshift32 */
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	state->state_array[idx] = x;
	state->state_index = (idx + 1) % 624;
	return (x);
}

/* Return PID as newly allocated string. Simpler and safer than reading /proc. */
static inline char	*getpid_hack(void)
{
	pid_t	pid;
	char	buf[32];

	pid = getpid();
	snprintf(buf, sizeof(buf), "%d", (int)pid);
	return (strdup(buf));
}


static inline int	mini_atoi_base(char **str, int base, int len)
{
	int		i;
	int		res;
	char	*digits;
	char	*p;

	i = 0;
	res = 0;
	digits = "0123456789ABCDEF";
	while (**str && i < len)
	{
		p = ft_strchr(digits, ft_toupper(**str));
		if (!p || (p - digits) >= base)
			break ;
		res = res * base + (p - digits);
		i++;
		(*str)++;
	}
	return (res);
}

static inline  void	parse_numeric_escape(char **str)
{
	int				base;
	int				len;
	unsigned char	c;

	base = 10;
	len = 0;
	if (**str == '0')
	{
		base = 8;
		len = 3;
	}
	else if (**str == 'x')
	{
		base = 16;
		len = 2;
	}
	else
		return ;
	(*str)++;
	c = mini_atoi_base(str, base, len);
	write(1, &c, 1);
}

static inline int backslash_writer(char *s)
{
	static bool	inited = false;
	static t_hash	map;

	const char *keys[] = {"n", "t", "a", "b", "f", "r", "v", "\\", "e"};
	const char *vals[] = {"\n", "\t", "\a", "\b", "\f", "\r", "\v", "\\", "\033"};
	char		keybuf[2];
	const char	*out;

	if (s == NULL || *s == '\0')
		return (ST_BASE_ERR);
	if (!inited)
	{
		if (!hash_init(&map, 16))
			return (ST_BASE_ERR);
		for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i)
			hash_set(&map, keys[i], (void *)vals[i]);
		inited = true;
	}
	keybuf[0] = *s;
	keybuf[1] = '\0';
	out = (const char *)hash_get(&map, keybuf);
	if (!out)
		return (ST_BASE_ERR);
	write(1, out, strlen(out));
	return (ST_OK);
}

static inline int	e_parser(char *s)
{
	while (*s)
	{
		if (*s == '\\' && s[1])
		{
			s++;
			if (*s == 'c')
				return (ST_BASE_ERR); /* signal caller to stop output */
			else if (*s == '0' || *s == 'x')
			{
				parse_numeric_escape(&s);
				continue ;
			}
			else if (backslash_writer(s) != ST_OK)
			{
				write(1, "\\", 1);
				write(1, s, 1);
			}
			s++;
		}
		else
			write(1, s++, 1);
	}
	return (ST_OK);
}

static inline oid	exit_clean(t_state *state, int code)
{
	char	*pid_s;

	pid_s = getpid_hack();
	if (pid_s && state->pid && ft_strcmp(state->pid, pid_s) == 0)
	{
		manage_history(state);
		free_all_state(state);
	}
	free(pid_s);
	exit(code);
}

static inline bool	is_valid_ident(char *id)
{
	int	i;

	i = 0;
	if (!is_var_name_p1(id[0]))
		return (false);
	while (id[i] && is_var_name_p2(id[i]))
		i++;
	return (!id[i]);
}

static inline bool	is_var_name_p1(char c)
{
	if (ft_isalpha(c) || c == '_')
		return (true);
	return (false);
}

//[a-zA-Z0-9_]
static inline bool	is_var_name_p2(char c)
{
	if (ft_isalnum(c) || c == '_')
		return (true);
	return (false);
}

static inline int	write_to_file(char *str, int fd)
{
	int	wrote_total;
	int	wrote;
	int	len;

	len = ft_strlen(str);
	wrote_total = 0;
	while (wrote_total != len)
	{
		wrote = write(fd, str + wrote_total, len - wrote_total);
		if (wrote < 0)
			return (1);
		wrote_total += wrote;
	}
	return (0);
}

static inline void	ft_assert(int cond)
{
	volatile char	*ft_nullptr;

	if (!cond)
	{
		ft_nullptr = 0;
		*ft_nullptr = 42;
	}
}

# ifndef MATH_H
#  define MATH_H

static inline ssize_t	ft_abs(ssize_t n)
{
	if (n < 0)
		return (-n);
	else
		return (n);
}

static inline ssize_t	ft_smin(ssize_t a, ssize_t b)
{
	return ((a > b) * b + (a <= b) * a);
}

static inline size_t	ft_min(size_t a, size_t b)
{
	return ((a > b) * b + (a <= b) * a);
}

static inline size_t	ft_max(size_t a, size_t b)
{
	return ((a < b) * b + (a >= b) * a);
}

static inline ssize_t	ft_smax(ssize_t a, ssize_t b)
{
	return ((a < b) * b + (a >= b) * a);
}

static inline ssize_t	ft_smod(ssize_t a, ssize_t b)
{
	if (a > 0)
		return (a % b);
	else
		return ((b + (a % b)) % b);
}

static inline int	ft_strnlen(char *s, int n)
{
	int	i;

	i = 0;
	while (i < n && s[i])
		i++;
	return (i);
}

static inline void	ft_fdputmem(int fd, char *s, int n)
{
	int	i;
	int	written;

	written = 0;
	while (written < n)
	{
		i = write(fd, s + written, n - written);
		if (i <= 0)
			break ;
		written += i;
	}
}

static inline void	ft_putmem(char *s, int n)
{
	ft_fdputmem(1, s, n);
}

static inline void	ft_eputmem(char *s, int n)
{
	ft_fdputmem(2, s, n);
}

static inline bool	str_slice_eq_str(char *s, size_t len, char *s2)
{
	size_t	len_s2;

	len_s2 = ft_strlen(s2);
	if (len != len_s2)
		return (false);
	if (ft_strncmp(s, s2, len) == 0)
		return (true);
	return (false);
}

#endif
# endif