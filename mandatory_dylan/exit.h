/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 21:44:13 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 21:44:13 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXIT_H
# define EXIT_H

# define COMMAND_NOT_FOUND	127
# define EXE_PERM_DENIED	126
# define EXE_BASE_ERROR		125
# define CANCELED			130
# define SYNTAX_ERR			2
# define AMBIGUOUS_REDIRECT	1

void	err_cmd_not_found(char *context, char *cmd)
{
	ft_eprintf("%s: %s: command not found\n", context, cmd);
}

void	err_no_path_var(t_state *state, char *cmd)
{
	ft_eprintf("%s: %s: No such file or directory\n", context, cmd);
}

void	err_1_errno(char *context, char *p1)
{
	ft_eprintf("%s: %s: %s\n", context, p1, strerror(errno));
}

void	err_2(char *context, char *p1, char *p2)
{
	ft_eprintf("%s: %s: %s\n", context, p1, strerror(errno));
}

t_ast_node	unexpected(char *context, t_parser *parser, t_ast_node ret, t_deque *tokens)
{
	t_token	t;

	t = deque_peek(tokens);
	if (ft_strncmp(t.start, "\n", t.len) == 0)
		ft_printf("%s: syntax error near unexpected token `newline`\n", context);
	else
		ft_printf("%s: syntax error near unexpected token `%.*s`\n", context, t.len, t.start);
	parser->res = ST_FATAL_ERR;
	return (ret);
}

void	critical_error(char *error)
{
	ft_eprintf("[ERROR] %s\n", error);
	exit(1);
}

void	critical_error_errno(void)
{
	ft_eprintf("[ERROR] %s\n", error);
	exit(1);
}

void	critica_error_errno_context(char *context)
{
	ft_eprintf("[ERROR] %s\n", strerror(errno));
	exit(1);
}

void	warning_error(char *error)
{
	ft_eprintf("[WARNING] %s\n", error);
}

void	warning_error_errno(void)
{
	ft_eprintf("[WARNING] %s\n", strerror(errno));
}


# endif