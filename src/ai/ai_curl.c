/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_curl.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"
#include <unistd.h>
#include <sys/wait.h>

/* In the child: stdin <- in[0], stdout <- op[1], then exec curl. */
static void	curl_child(int *in, int *op, char *const *argv)
{
	dup2(in[0], 0);
	dup2(op[1], 1);
	close(in[0]);
	close(in[1]);
	close(op[0]);
	close(op[1]);
	execvp("curl", argv);
	_exit(127);
}

/* Drain the child's stdout into out (capped, NUL-terminated). */
static size_t	read_out(int fd, char *out, size_t cap)
{
	ssize_t	r;
	size_t	len;

	len = 0;
	r = read(fd, out, cap - 1);
	while (r > 0 && len < cap - 1)
	{
		len += (size_t)r;
		r = read(fd, out + len, cap - 1 - len);
	}
	out[len] = '\0';
	return (len);
}

/* curl -fs --max-time S --connect-timeout 3 -X POST URL -H ct [-H h]... -d @-
   (body on stdin). -f fails (exit 22) on HTTP >=400 so we fall back; -s keeps
   it quiet; --connect-timeout bounds a slow/hung TLS handshake separately from
   the overall budget. hdrs is a NULL-terminated list of extra headers. */
static void	build_argv(char **a, const char *url, char **hdrs, const char *secs)
{
	int	i;
	int	j;

	i = 0;
	a[i++] = "curl";
	a[i++] = "-fs";
	a[i++] = "--max-time";
	a[i++] = (char *)secs;
	a[i++] = "--connect-timeout";
	a[i++] = "3";
	a[i++] = "-X";
	a[i++] = "POST";
	a[i++] = (char *)url;
	a[i++] = "-H";
	a[i++] = "Content-Type: application/json";
	j = 0;
	while (hdrs[j])
	{
		a[i++] = "-H";
		a[i++] = hdrs[j++];
	}
	a[i++] = "-d";
	a[i++] = "@-";
	a[i] = NULL;
}

/* Run argv feeding body on stdin, capture stdout into out. Returns curl's exit
   status (0 ok; curl -f => non-zero on HTTP >=400, e.g. 429 out-of-quota). */
static int	run_curl(char **argv, const char *body, char *out, size_t cap)
{
	int	in[2];
	int	op[2];
	int	pid;
	int	st;

	if (pipe(in) || pipe(op))
		return (-1);
	pid = fork();
	if (pid < 0)
		return (-1);
	if (pid == 0)
		curl_child(in, op, argv);
	close(in[0]);
	close(op[1]);
	if (write(in[1], body, ft_strlen(body)) < 0)
		out[0] = '\0';
	close(in[1]);
	read_out(op[0], out, cap);
	close(op[0]);
	waitpid(pid, &st, 0);
	if (WIFEXITED(st))
		return (WEXITSTATUS(st));
	return (-1);
}

/* POST body to url via curl (Bearer key if set); response body (xfree) or NULL.
   This is the whole cloud path: TLS + auth for free, works with Groq / OpenAI /
   OpenRouter / any OpenAI-compatible endpoint. */
char	*ai_curl(const char *url, const char *key, const char *body, int tmo)
{
	char	*out;
	char	*argv[24];
	char	*hdrs[3];
	char	auth[AI_AUTH_CAP];
	char	*secs;

	ai_auth_headers(url, key, auth, hdrs);
	if (tmo < 1000)
		tmo = 1000;
	secs = ft_itoa(tmo / 1000);
	build_argv(argv, url, hdrs, secs);
	out = xmalloc(AI_MAX_REPLY + 1);
	if (run_curl(argv, body, out, AI_MAX_REPLY + 1) != 0 || !out[0])
		return (xfree(secs), xfree(out), NULL);
	return (xfree(secs), out);
}
