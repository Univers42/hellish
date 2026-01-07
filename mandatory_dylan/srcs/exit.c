/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exit.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 15:44:59 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 15:44:59 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../incs/exit.h"

void	err_cmd_not_found(const char *context, const char *cmd)
{
	ft_eprintf("%s: %s: command not found\n", context, cmd);
}

void	err_no_path_var(const char *context, const char *cmd)
{
	ft_eprintf("%s: %s: No such file or directory\n", context, cmd);
}

void	err_1_errno(const char *context, const char *p1)
{
	ft_eprintf("%s: %s: %s\n", context, p1, strerror(errno));
}

void	err_2(const char *context, const char *p1, const char *p2)
{
	(void)p2;
	ft_eprintf("%s: %s: %s\n", context, p1, strerror(errno));
}

void	unexpected_simple(const char *context, const char *tok, int tok_len)
{
	if (tok && tok_len == 1 && tok[0] == '\n')
		ft_printf("%s: syntax error near unexpected token `newline`\n", context);
	else if (tok && tok_len > 0)
		ft_printf("%s: syntax error near unexpected token `%.*s`\n", context, tok_len, tok);
}

void	critical_error(const char *error)
{
	ft_eprintf("[ERROR] %s\n", error);
	exit(1);
}

void	critical_error_errno(void)
{
	ft_eprintf("[ERROR] %s\n", strerror(errno));
	exit(1);
}

void	critica_error_errno_context(const char *context)
{
	ft_eprintf("[ERROR] %s: %s\n", context, strerror(errno));
	exit(1);
}

void	warning_error(const char *error)
{
	ft_eprintf("[WARNING] %s\n", error);
}

void	warning_error_errno(void)
{
	ft_eprintf("[WARNING] %s\n", strerror(errno));
}
