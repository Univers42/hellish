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

/* The configured model, but only for the cloud stage -- the local fallback
   passes none so llama-server uses whatever it loaded. */
static const char	*cloud_model(int cloud)
{
	if (!cloud)
		return (NULL);
	return (getenv("HELLISH_AI_MODEL"));
}

/* Try one stage of the provider chain. cloud=1 reads the configured endpoint
   ($HELLISH_AI_URL/KEY/MODEL) and goes via curl (TLS + auth); cloud=0 is the
   native local POST with no model. The body shape, system prompt, decoding
   params, and reply key follow the detected provider and task (as_cmd). Reply
   xfree'd; NULL on any failure so the caller can fall back. */
static char	*try_provider(const char *user, int as_cmd, int cloud)
{
	char	*url;
	char	*body;
	char	*resp;
	char	*out;
	int		prov;

	url = NULL;
	if (cloud)
		url = getenv("HELLISH_AI_URL");
	prov = ai_provider(url);
	body = ai_body(prov, cloud_model(cloud), as_cmd, user);
	if (url && *url)
		resp = ai_curl(url, getenv("HELLISH_AI_KEY"), body, ai_timeout_ms());
	else
		resp = ai_post("/v1/chat/completions", body, ai_timeout_ms());
	xfree(body);
	if (!resp)
		return (NULL);
	out = ai_json_get_str(resp, ai_resp_key(prov));
	return (xfree(resp), out);
}

/* The brain: prepend shell context (a lite cut for completions, rich for chat)
   to the instruction, then walk the provider chain -- a configured cloud
   primary ($HELLISH_AI_URL) first, then the local server -- so cloud answers
   when it can and we fall back to local on any error / rate-limit (429). Strips
   a markdown fence when as_cmd. */
char	*ai_request(const char *instruction, int as_cmd)
{
	char	*ctx;
	char	*user;
	char	*out;

	out = ai_context_for(as_cmd);
	ctx = ft_strjoin(out, "\nUser request: ");
	xfree(out);
	user = ft_strjoin(ctx, instruction);
	xfree(ctx);
	out = NULL;
	if (getenv("HELLISH_AI_URL"))
		out = try_provider(user, as_cmd, 1);
	if (!out)
		out = try_provider(user, as_cmd, 0);
	xfree(user);
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
