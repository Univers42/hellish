/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 15:44:28 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 15:44:28 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "utils.h"

bool is_space(int c)
{
	return (c == ' ' || c == '\t');
}

bool is_special_char(int c)
{
	const char	*specials = ";$'\"<>|&()\n";

	return (strchr(specials, c) != NULL || is_space(c));
}

/* Initialize pseudo-random generator state.
   Uses MT19937-style seeding to fill the state array deterministically. */
void	prng_init_state(t_prng_state *state, uint32_t seed)
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
uint32_t	random_uint32(t_prng_state *state)
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
char	*getpid_hack(void)
{
	pid_t	pid;
	char	buf[32];

	pid = getpid();
	snprintf(buf, sizeof(buf), "%d", (int)pid);
	return (strdup(buf));
}


int	mini_atoi_base(char **str, int base, int len)
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

 void	parse_numeric_escape(char **str)
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

/* Simple backslash map implementation — no hash table dependency. */
/* returns 0 on success, non-zero on error */
int backslash_writer(char *s)
{
	if (s == NULL || *s == '\0')
		return (1);
	switch (*s)
	{
		case 'n': write(1, "\n", 1); return (0);
		case 't': write(1, "\t", 1); return (0);
		case 'a': write(1, "\a", 1); return (0);
		case 'b': write(1, "\b", 1); return (0);
		case 'f': write(1, "\f", 1); return (0);
		case 'r': write(1, "\r", 1); return (0);
		case 'v': write(1, "\v", 1); return (0);
		case '\\': write(1, "\\", 1); return (0);
		case 'e': write(1, "\033", 1); return (0);
		default:
			return (1);
	}
}

/* e_parser: return 0 on success, non-zero to signal caller to stop/output error */
int	e_parser(char *s)
{
	while (*s)
	{
		if (*s == '\\' && s[1])
		{
			s++;
			if (*s == 'c')
				return (1); /* signal caller to stop output */
			else if (*s == '0' || *s == 'x')
			{
				parse_numeric_escape(&s);
				continue ;
			}
			else if (backslash_writer(s) != 0)
			{
				write(1, "\\", 1);
				write(1, s, 1);
			}
			s++;
		}
		else
			write(1, s++, 1);
	}
	return (0);
}

/* forward prototypes so is_valid_ident can call them before their definition */


bool	is_valid_ident(char *id)
{
	int	i;

	i = 0;
	if (!is_var_name_p1(id[0]))
		return (false);
	while (id[i] && is_var_name_p2(id[i]))
		i++;
	return (!id[i]);
}

bool	is_var_name_p1(char c)
{
	if (ft_isalpha(c) || c == '_')
		return (true);
	return (false);
}

//[a-zA-Z0-9_]
bool	is_var_name_p2(char c)
{
	if (ft_isalnum(c) || c == '_')
		return (true);
	return (false);
}

int	write_to_file(char *str, int fd)
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

void	ft_assert(int cond)
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

ssize_t	ft_abs(ssize_t n)
{
	if (n < 0)
		return (-n);
	else
		return (n);
}

ssize_t	ft_smin(ssize_t a, ssize_t b)
{
	return ((a > b) * b + (a <= b) * a);
}

size_t	ft_min(size_t a, size_t b)
{
	return ((a > b) * b + (a <= b) * a);
}

size_t	ft_max(size_t a, size_t b)
{
	return ((a < b) * b + (a >= b) * a);
}

ssize_t	ft_smax(ssize_t a, ssize_t b)
{
	return ((a < b) * b + (a >= b) * a);
}

ssize_t	ft_smod(ssize_t a, ssize_t b)
{
	if (a > 0)
		return (a % b);
	else
		return ((b + (a % b)) % b);
}

int	ft_strnlen(char *s, int n)
{
	int	i;

	i = 0;
	while (i < n && s[i])
		i++;
	return (i);
}

void	ft_fdputmem(int fd, char *s, int n)
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

void	ft_putmem(char *s, int n)
{
	ft_fdputmem(1, s, n);
}

void	ft_eputmem(char *s, int n)
{
	ft_fdputmem(2, s, n);
}

bool	str_slice_eq_str(char *s, size_t len, char *s2)
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