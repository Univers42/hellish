/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/26 17:57:01 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/02 17:52:31 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

#include "lexer.h"
// -----------------------------------------------------------------

#define FMT_TOKEN "|@$%&/()=?¿'{}[]^*`+.,-;:"
#define BIT_BUFFER 8

// --- 2. Your Initialization Helpers (Unchanged) ---
static void init_op_and_redir(t_fmt *table)
{
	table['|'] = handle_pipe;
	table['&'] = handle_ampersand;
	table[';'] = handle_semicolon;
	table['\n'] = handle_newline;
	table['<'] = handle_less;
	table['>'] = handle_greater;
}

static void init_arith_and_group(t_fmt *table)
{
	table['+'] = handle_plus;
	table['-'] = handle_minus;
	table['*'] = handle_star;
	table['/'] = handle_div;
	table['%'] = handle_percent;
	table['='] = handle_eq;
	table[':'] = handle_colon; // add colon handler
	table['('] = handle_lparen;
	table[')'] = handle_rparen;
	table['{'] = handle_lbrace;
	table['}'] = handle_rbrace;
}

static void init_exp_and_glob(t_fmt *table)
{
	table['\''] = handle_squote;
	table['"'] = handle_dquote;
	table['`'] = handle_bquote;
	table['\\'] = handle_backslash;
	table['$'] = handle_dollar;
	table['~'] = handle_tilde;
	table['#'] = handle_hash;
	table['!'] = handle_bang;
	table['?'] = handle_glob;
	table['['] = handle_glob;
	table[']'] = handle_glob;
}

// --- 3. The Singleton Function ---
const t_dispatch_data *get_dispatch_singleton(void)
{
	static t_dispatch_data data;
	static int init = 0;
	t_fmt *table_ptr;
	int i;

	if (init)
		return (&data);

	// 1. Initialize the 128-entry table (not 256)
	table_ptr = data.dispatch_table;
	i = -1;
	while (++i < 128)
		table_ptr[i] = handle_literal;

	// 2. Populate special chars using your helpers
	init_op_and_redir(table_ptr);
	init_arith_and_group(table_ptr);
	init_exp_and_glob(table_ptr);

	table_ptr[' '] = handle_whitespace;
	table_ptr['\t'] = handle_whitespace;
	table_ptr['.'] = handle_dot;
	table_ptr[0] = handle_sys_op; // EOF

	// 3. Set the 8-BYTE (64-bit) fast-path map.
	// Compute the map dynamically from the dispatch table to avoid
	// mismatch between table entries and hardcoded bitmask.
	{
		uint64_t map = 0ULL;
		int j = 0;
		while (j < 64)
		{
			if (table_ptr[j] != handle_literal)
				map |= (1ULL << j);
			j++;
		}
		data.fast_path_map = map;
	}
	init = 1;
	return (&data);
}
