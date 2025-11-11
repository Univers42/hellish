#include "lexer.h"
#include <stdio.h>
#include <string.h> // for strlen, memcmp if needed

t_token scan_token(t_scanner *scan)
{
	skip_unused_data(scan);
	scan->start = scan->current;
	if (scan_is_at_end(scan))
		return make_token(scan, TOKEN_EOF);
	const t_dispatch_data *dispatch = get_dispatch_singleton();
	unsigned char c = (unsigned char)*scan->current;

	/* Fast path for common literal chars */
	if (c < 64 && !(dispatch->fast_path_map & (1ULL << c)))
		return handle_literal(scan);

	/* Alphabetic or underscore -> identifier */
	if (ft_isalpha(c) || c == '_')
		return identifier(scan);

	/* Digit -> number */
	if (ft_isdigit(c))
		return number(scan);

	if (c < 128)
		return dispatch->dispatch_table[c](scan);
	return handle_literal(scan);
}

void scan_all_tokens(t_scanner *scan, int debug)
{
	t_token token;

	token = (t_token){0};
	while (token.type != TOKEN_EOF && token.type != TOKEN_ERR)
	{
		token = scan_token(scan);
		if (debug)
			print_token(&token);
	}
}

void print_token(const t_token *token)
{
	const char	*name = "UNKNOWN";
	int			i;

	i = 0;
	while (tok[i].rep_name != NULL)
	{
		if (tok[i].token == token->type)
		{
			name = tok[i].rep_name;
			break ;
		}
		i++;
	}
	printf(CYAN_TERM" %-27s "RESET_TERM"|"BLACK_TERM" line %d "RESET_TERM"|"GREEN_TERM BOLD_TERM " %.*s\n"RESET_TERM,
			name, token->line, token->length, token->start);
}

