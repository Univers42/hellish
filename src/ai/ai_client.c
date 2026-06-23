/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_client.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"
#include <unistd.h>
#include <stdlib.h>

/* The JSON tail after the user message: inject the model when one is given,
   else let llama-server use whatever it loaded. */
static char	*chat_tail(const char *model)
{
	char	*a;
	char	*b;

	if (!model || !*model)
		return (ft_strdup("\"}],\"max_tokens\":256,\"stream\":false}"));
	a = ft_strjoin("\"}],\"model\":\"", model);
	b = ft_strjoin(a, "\",\"max_tokens\":256,\"stream\":false}");
	xfree(a);
	return (b);
}

/* Build the chat request body for llama.cpp's OpenAI-compatible endpoint. */
static char	*chat_body(const char *model, const char *msg)
{
	char	*esc;
	char	*head;
	char	*tail;
	char	*body;

	esc = ai_json_escape(msg);
	head = ft_strjoin("{\"messages\":[{\"role\":\"user\",\"content\":\"", esc);
	xfree(esc);
	tail = chat_tail(model);
	body = ft_strjoin(head, tail);
	xfree(head);
	xfree(tail);
	return (body);
}

/* Try one provider: build the body and send it -- a configured cloud endpoint
   (url set) goes via curl for TLS + Bearer auth; otherwise the native local
   POST. Returns the assistant text (xfree it) or NULL on any failure. */
static char	*try_provider(const char *url, const char *key,
		const char *model, const char *msg)
{
	char	*body;
	char	*resp;
	char	*out;

	body = chat_body(model, msg);
	if (url && *url)
		resp = ai_curl(url, key, body, ai_timeout_ms());
	else
		resp = ai_post("/v1/chat/completions", body, ai_timeout_ms());
	xfree(body);
	if (!resp)
		return (NULL);
	out = ai_json_get_str(resp, "content");
	return (xfree(resp), out);
}

/* The brain: prepend rich shell context to the instruction, then walk the
   provider chain -- a configured cloud primary ($HELLISH_AI_URL, e.g. Groq)
   first, then the local server -- so cloud answers when it can and we fall back
   to local on any error / rate-limit (429). The local stage passes no model so
   llama-server uses whatever it loaded. Strips a markdown fence when as_cmd. */
char	*ai_request(const char *instruction, int as_cmd)
{
	char	*ctx;
	char	*msg;
	char	*out;
	char	*url;

	out = ai_context();
	ctx = ft_strjoin(out, "\nUser request: ");
	xfree(out);
	msg = ft_strjoin(ctx, instruction);
	xfree(ctx);
	out = NULL;
	url = getenv("HELLISH_AI_URL");
	if (url && *url)
		out = try_provider(url, getenv("HELLISH_AI_KEY"),
				getenv("HELLISH_AI_MODEL"), msg);
	if (!out)
		out = try_provider(NULL, NULL, NULL, msg);
	xfree(msg);
	if (out && as_cmd)
		ai_strip_fence(out);
	return (out);
}

/* 0 when a TCP connect to the server succeeds within 1s.
   ponytail: connect-probe, not a real /health round-trip -- enough to tell
   "server up" for status and gating. */
int	ai_reachable(void)
{
	int	fd;

	fd = ai_connect(ai_host(), ai_port(), 1000);
	if (fd < 0)
		return (-1);
	close(fd);
	return (0);
}
