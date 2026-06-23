/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_ai_setup.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_input.h"

char	*ai_rc_read(void);
int		ai_rc_write(const char *content);
void	read_secret(char *buf, size_t n);

#define RC_START "# >>> hellish ai (managed by 'ai setup') >>>\n"
#define RC_END "# <<< hellish ai <<<\n"

/* Resolve a known cloud provider to its OpenAI-compatible endpoint + a good
   default model. Returns 1 if known, 0 otherwise. */
static int	preset(const char *name, char **url, char **model)
{
	if (!ft_strcmp(name, "groq"))
		return (*url = "https://api.groq.com/openai/v1/chat/completions",
			*model = "llama-3.3-70b-versatile", 1);
	if (!ft_strcmp(name, "openrouter"))
		return (*url = "https://openrouter.ai/api/v1/chat/completions",
			*model = "anthropic/claude-3.5-sonnet", 1);
	if (!ft_strcmp(name, "openai"))
		return (*url = "https://api.openai.com/v1/chat/completions",
			*model = "gpt-4o-mini", 1);
	return (0);
}

/* Build the managed ~/.hellishrc block. url==NULL => local-only (just the
   toggle). The block is delimited by markers so a re-run can replace it. */
static char	*build_block(const char *url, const char *model, const char *key)
{
	char	*b;
	size_t	cap;

	cap = 8192;
	b = xmalloc(cap);
	ft_strlcpy(b, RC_START, cap);
	ft_strlcat(b, "set -o ai\n", cap);
	if (url)
	{
		ft_strlcat(b, "export HELLISH_AI_URL=", cap);
		ft_strlcat(b, url, cap);
		ft_strlcat(b, "\nexport HELLISH_AI_KEY=", cap);
		ft_strlcat(b, key, cap);
		ft_strlcat(b, "\nexport HELLISH_AI_MODEL=", cap);
		ft_strlcat(b, model, cap);
		ft_strlcat(b, "\n", cap);
	}
	return (ft_strlcat(b, RC_END, cap), b);
}

/* Return content with any previously-managed block removed (xfree it), so a
   re-run replaces the old config instead of stacking duplicates. */
static char	*splice_rc(const char *content)
{
	char	*s;
	char	*e;
	char	*before;
	char	*out;

	s = ft_strnstr(content, RC_START, ft_strlen(content));
	if (!s)
		return (ft_strdup(content));
	e = ft_strnstr(s, RC_END, ft_strlen(s));
	if (!e)
		return (ft_substr(content, 0, s - content));
	e += ft_strlen(RC_END);
	before = ft_substr(content, 0, (size_t)(s - content));
	out = ft_strjoin(before, e);
	return (xfree(before), out);
}

/* Rewrite ~/.hellishrc: drop the old managed block, append the fresh one. */
static int	apply_config(const char *url, const char *model, const char *key)
{
	char	*content;
	char	*kept;
	char	*block;
	char	*whole;

	content = ai_rc_read();
	kept = splice_rc(content);
	xfree(content);
	block = build_block(url, model, key);
	whole = ft_strjoin(kept, block);
	xfree(kept);
	xfree(block);
	if (ai_rc_write(whole) != 0)
		return (xfree(whole), -1);
	return (xfree(whole), 0);
}

/* `ai setup <groq|openrouter|openai|local> [key]`: write the provider block to
   ~/.hellishrc. The key is taken from the arg, else prompted (hidden) when
   interactive. Re-runnable: it replaces the previous managed block. */
int	cmd_ai_setup(t_shell *state, char **av, size_t n)
{
	char	*url;
	char	*model;
	char	key[512];

	url = NULL;
	model = NULL;
	if (n == 0 || (ft_strcmp(av[0], "local") && !preset(av[0], &url, &model)))
		return (ft_eprintf("usage: ai setup groq|openrouter|openai|local "
				"[apikey]\n"), 2);
	key[0] = '\0';
	if (n >= 2)
		ft_strlcpy(key, av[1], sizeof(key));
	else if (url && state->metinp == INP_RL)
	{
		ft_eprintf("%s API key (hidden, Enter to skip): ", av[0]);
		read_secret(key, sizeof(key));
	}
	if (apply_config(url, model, key) != 0)
		return (ft_eprintf("ai setup: could not write ~/.hellishrc\n"), 1);
	if (url && !key[0])
		return (ft_eprintf("ai setup: %s saved, but NO key was entered -- it "
				"won't work. Re-run:  ai setup %s <your-key>\n", av[0],
				av[0]), 1);
	return (ft_printf("ai: configured (%s). run: source ~/.hellishrc\n",
			av[0]), 0);
}
