/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_http.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"
#include <unistd.h>

/* Append host port + the fixed headers + the blank-line terminator. */
static void	append_headers(char *req, size_t cap, size_t blen)
{
	char	*port;
	char	*len;

	port = ft_itoa(ai_port());
	len = ft_itoa((int)blen);
	ft_strlcat(req, ":", cap);
	ft_strlcat(req, port, cap);
	ft_strlcat(req, "\r\nContent-Type: application/json\r\n", cap);
	ft_strlcat(req, "Content-Length: ", cap);
	ft_strlcat(req, len, cap);
	ft_strlcat(req, "\r\nConnection: close\r\n\r\n", cap);
	xfree(port);
	xfree(len);
}

/* Compose the full request in one allocation. Connection: close lets
   ai_read_all stop at EOF without parsing Content-Length. */
char	*ai_build_request(const char *path, const char *body)
{
	char	*req;
	char	*host;
	size_t	cap;

	host = ai_host();
	cap = ft_strlen(path) + ft_strlen(body) + ft_strlen(host) + 256;
	req = xmalloc(cap);
	req[0] = '\0';
	ft_strlcat(req, "POST ", cap);
	ft_strlcat(req, path, cap);
	ft_strlcat(req, " HTTP/1.1\r\nHost: ", cap);
	ft_strlcat(req, host, cap);
	append_headers(req, cap, ft_strlen(body));
	ft_strlcat(req, body, cap);
	return (req);
}

/* POST body to path; return the response body (after the blank line) as a
   fresh string, or NULL on any failure. */
char	*ai_post(const char *path, const char *body, int timeout_ms)
{
	int		fd;
	char	*req;
	char	*resp;
	char	*out;

	fd = ai_connect(ai_host(), ai_port(), timeout_ms);
	if (fd < 0)
		return (NULL);
	req = ai_build_request(path, body);
	if (ai_send_all(fd, req, ft_strlen(req)) < 0)
		return (xfree(req), close(fd), NULL);
	xfree(req);
	resp = ai_read_all(fd);
	close(fd);
	out = ft_strnstr(resp, "\r\n\r\n", ft_strlen(resp));
	if (!out)
		return (xfree(resp), NULL);
	out = ft_strdup(out + 4);
	return (xfree(resp), out);
}

/* GET path (no body); return the response body (xfree it) or NULL. */
char	*ai_get(const char *path, int timeout_ms)
{
	int		fd;
	char	*req;
	char	*resp;
	char	*out;

	fd = ai_connect(ai_host(), ai_port(), timeout_ms);
	if (fd < 0)
		return (NULL);
	req = ft_strjoin("GET ", path);
	out = ft_strjoin(req, " HTTP/1.1\r\nConnection: close\r\n\r\n");
	xfree(req);
	if (ai_send_all(fd, out, ft_strlen(out)) < 0)
		return (xfree(out), close(fd), NULL);
	xfree(out);
	resp = ai_read_all(fd);
	close(fd);
	out = ft_strnstr(resp, "\r\n\r\n", ft_strlen(resp));
	if (!out)
		return (xfree(resp), NULL);
	out = ft_strdup(out + 4);
	return (xfree(resp), out);
}
