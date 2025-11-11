#include "lexer.h"

t_token_type check_key_word(int start, int length, const char *rest, t_token_type type, t_scanner *scan);

t_token handle_pipe(t_scanner *scan)
{
	advance(scan);
	if (match(scan, '|'))
		return (make_token(scan, TOKEN_PIPE_PIPE));
	else if (match(scan, '='))
		return (make_token(scan, TOKEN_ASS_OR));
	else
		return (make_token(scan, TOKEN_PIPE));
}

t_token handle_ampersand(t_scanner *scan)
{
	advance(scan);
	if (match(scan, '&'))
		return (make_token(scan, TOKEN_AMP_AMP));
	else
		return (make_token(scan, TOKEN_AMPERSAND));
}

t_token handle_semicolon(t_scanner *scan)
{
	advance(scan);
	if (match(scan, ';'))
		return (make_token(scan, TOKEN_SEMICOLON));
	else if (match(scan, '\n'))
		return (make_token(scan, TOKEN_NEWLINE));
	/* Single semicolon -> return semicolon token */
	return (make_token(scan, TOKEN_SEMICOLON));
}

t_token handle_glob(t_scanner *scan)
{
	char terminator;

	terminator = peek(scan);
	advance(scan);
	if (terminator == '?')
		return (make_token(scan, TOKEN_GLOB_QUESTION)); // was TOKEN_QUESTION
	if (terminator == '[')
		return (make_token(scan, TOKEN_GLOB_BRACKET_OPEN));
	else if (terminator == ']')
		return (make_token(scan, TOKEN_GLOB_BRACKET_CLOSE));
	return (error_token(scan, "Error glob token"));
}

t_token handle_minus(t_scanner *scan)
{
    /* consume initial '-' */
    advance(scan);

    /* Helper: stop on whitespace or shell metacharacters */
#define IS_METACHAR(c) ( (c) == '|' || (c) == '&' || (c) == ';' || (c) == '<' || (c) == '>' || \
                        (c) == '(' || (c) == ')' || (c) == '{' || (c) == '}' || \
                        (c) == '\'' || (c) == '"' || (c) == '`' || (c) == '\\' || \
                        (c) == '$' || (c) == '#' || (c) == '~' || (c) == '*' || \
                        (c) == '?' || (c) == '[' || (c) == ']' )

    /* Double-dash long flag: consume until whitespace or metachar */
    if (match(scan, '-'))
    {
        while (!scan_is_at_end(scan))
        {
            char c = peek(scan);
            if (ft_isspace(c) || IS_METACHAR(c))
                break;
            advance(scan);
        }
        return (make_token(scan, TOKEN_FLAG));
    }

    /* Single-dash: if it's just a lone '-' operator, return OP_MINUS */
    if (ft_isspace(peek(scan)) || IS_METACHAR(peek(scan)) || scan_is_at_end(scan))
        return (make_token(scan, TOKEN_OP_MINUS));

    /* Otherwise treat as a short flag that may include any non-whitespace/non-metachar chars (including '/') */
    while (!scan_is_at_end(scan))
    {
        char c = peek(scan);
        if (ft_isspace(c) || IS_METACHAR(c))
            break;
        advance(scan);
    }
    return (make_token(scan, TOKEN_FLAG));
#undef IS_METACHAR
}

t_token handle_greater(t_scanner *scan)
{
	advance(scan);
	/* Process substitution >( */
	if (match(scan, '('))
		return (make_token(scan, TOKEN_SUBST_OUT));
	/* Append redirection >> (shell) */
	if (match(scan, '>'))
	{
		return (make_token(scan, TOKEN_REDIR_APPEND));
	}
	/* FD redirection '>&' (duplicate output FD) */
	if (match(scan, '&'))
		return (make_token(scan, TOKEN_REDIR_FD_OUT));
	/* Clobber >| */
	if (match(scan, '|'))
		return (make_token(scan, TOKEN_REDIR_CLOBBER));
	/* Default: single '>' => output redirection */
	return (make_token(scan, TOKEN_REDIR_OUT));
}

t_token handle_less(t_scanner *scan)
{
	advance(scan);
	/* Process substitution <( */
	if (match(scan, '('))
		return (make_token(scan, TOKEN_SUBST_IN));
	/* Heredoc << or <<- */
	if (match(scan, '<'))
	{
		if (match(scan, '-'))
			return (make_token(scan, TOKEN_REDIR_HEREDOC_STRIP));
		return (make_token(scan, TOKEN_REDIR_HEREDOC));
	}
	/* FD redirection '<&' */
	if (match(scan, '&'))
		return (make_token(scan, TOKEN_REDIR_FD_IN));
	/* In/out pair '<>' */
	if (match(scan, '>'))
		return (make_token(scan, TOKEN_REDIR_INOUT));
	/* <= is not a shell redirection here; ignore arithmetic comparator */
	/* Default: single '<' => input redirection */
	return (make_token(scan, TOKEN_REDIR_IN));
}

t_token handle_lparen(t_scanner *scan)
{
	advance(scan);
	return (make_token(scan, TOKEN_LEFT_PAREN));
}

t_token handle_rparen(t_scanner *scan)
{
	advance(scan);
	/* If we are in arithmetic mode and we see '))', reset mode on the first ')' */
	if (scan->lexer_state && peek(scan) == ')')
		scan->lexer_state = 0;
	return (make_token(scan, TOKEN_RIGHT_PAREN));
}

t_token handle_lbrace(t_scanner *scan)
{
	advance(scan);
	return (make_token(scan, TOKEN_LEFT_BRACE));
}

t_token handle_rbrace(t_scanner *scan)
{
	advance(scan);
	return (make_token(scan, TOKEN_RIGHT_BRACE));
}

t_token handle_newline(t_scanner *scan)
{
	advance(scan);
	return (make_token(scan, TOKEN_NEWLINE));
}

//<<< Category strings
//----------------------------------------------
t_token handle_squote(t_scanner *scan)
{
	return (string(scan));
}

t_token handle_dquote(t_scanner *scan)
{
	return (string(scan));
}

t_token handle_bquote(t_scanner *scan)
{
	return (string(scan));
}
//-----------------------------------------------
t_token handle_sys_op(t_scanner *scan)
{
	return (make_token(scan, TOKEN_EOF));
}

t_token handle_dollar(t_scanner *scan)
{
	advance(scan);
	/* Special parameters first */
	if (match(scan, '*'))
		return (make_token(scan, TOKEN_DOLLAR_STAR));
	if (match(scan, '#'))
		return (make_token(scan, TOKEN_DOLLAR_HASH));
	if (match(scan, '@'))
		return (make_token(scan, TOKEN_DOLLAR_AT));
	if (match(scan, '?'))
		return (make_token(scan, TOKEN_DOLLAR_QUESTION));
	if (match(scan, '$'))
		return (make_token(scan, TOKEN_DOLLAR_DOLLAR));
	if (match(scan, '!'))
		return (make_token(scan, TOKEN_DOLLAR_BANG));
	if (match(scan, '-'))
		return (make_token(scan, TOKEN_DOLLAR_DASH));
	if (match(scan, '0'))
		return (make_token(scan, TOKEN_DOLLAR_ZERO));
	/* Groupings and expansions */
	if (match(scan, '{'))
		return (make_token(scan, TOKEN_PAR_EXP_START));
	/* Command substitution $(...)/ Arithmetic expansion $((...)) */
	if (peek(scan) == '(')
	{
		if (peek_next(scan) == '(')
		{
			advance(scan);		   /* consume first '(' */
			advance(scan);		   /* consume second '(' */
			scan->lexer_state = 1; /* enter arithmetic mode */
			return (make_token(scan, TOKEN_ARITH_EXP_START));
		}
		advance(scan); /* consume '(' for $( */
		return (make_token(scan, TOKEN_CMD_SUBST_START));
	}
	/* Default: plain '$' prefix (e.g., $HOME) */
	return (make_token(scan, TOKEN_DOLLAR));
}

t_token handle_backslash(t_scanner *scan)
{
	advance(scan);
	return (make_token(scan, TOKEN_BACKSLASH));
}

/**
 * here different possible intepretation
 * TOKEN_HASH_OP
 * TOKEN_POUND
 * TOKEN_POUND_POUND
 * TOKEN_HASH
 */
t_token handle_hash(t_scanner *scan)
{
	advance(scan);
	if (match(scan, '#'))
		return (make_token(scan, TOKEN_POUND_POUND));
	return (make_token(scan, TOKEN_POUND));
}

t_token handle_tilde(t_scanner *scan)
{
	advance(scan);
	return (make_token(scan, TOKEN_TILDE));
}

t_token handle_star(t_scanner *scan)
{
	advance(scan);
	/* In arithmetic mode: '*' or '*=' */
	if (scan->lexer_state)
	{
		if (match(scan, '='))
			return (make_token(scan, TOKEN_ASS_MULT));
		return (make_token(scan, TOKEN_OP_MULT));
	}
	/* Non-arithmetic: globbing */
	if (match(scan, '*'))
		return (make_token(scan, TOKEN_GLOB_STAR));
	else if (match(scan, '='))
		return (make_token(scan, TOKEN_ASS_MULT));
	return (make_token(scan, TOKEN_GLOB_STAR));
}

t_token handle_plus(t_scanner *scan)
{
	advance(scan);
	if (match(scan, '+'))
		return (make_token(scan, TOKEN_OP_INCREMENT));
	else if (match(scan, '='))
		return (make_token(scan, TOKEN_ASS_PLUS));
	return (make_token(scan, TOKEN_OP_ADD));
}

t_token handle_div(t_scanner *scan)
{
	if (scan->lexer_state)
	{
		advance(scan);
		if (match(scan, '='))
			return (make_token(scan, TOKEN_ASS_DIV));
		return (make_token(scan, TOKEN_OP_DIV));
	}
	return (handle_word(scan));
}

t_token handle_percent(t_scanner *scan)
{
	advance(scan);
	/* In arithmetic mode: '%' or '%=' */
	if (scan->lexer_state)
	{
		if (match(scan, '='))
			return (make_token(scan, TOKEN_ASS_MOD));
		return (make_token(scan, TOKEN_OP_MOD));
	}
	/* Outside arithmetic: parameter expansion suffix ops */
	if (match(scan, '%'))
		return (make_token(scan, TOKEN_PERCENT_PERCENT));
	return (make_token(scan, TOKEN_PERCENT));
}

t_token handle_eq(t_scanner *scan)
{
	advance(scan);
	/* Handle '==' as comparison, '='+assignment, '+=' handled separately by handlers on '+' */
	if (match(scan, '='))
		return (make_token(scan, TOKEN_COMP_EQ));
	else if (match(scan, '+'))
		return (make_token(scan, TOKEN_ASS_PLUS));
	/* Single '=' assignment */
	return (make_token(scan, TOKEN_ASS_EQUAL));
}

/* New: colon handler for ${VAR:-word}, ${VAR:=word}, ${VAR:+word}, ${VAR:?word} */
t_token handle_colon(t_scanner *scan)
{
	advance(scan);
	if (match(scan, '-'))
		return (make_token(scan, TOKEN_COLON_DASH));
	else if (match(scan, '='))
		return (make_token(scan, TOKEN_COLON_EQUAL));
	else if (match(scan, '+'))
		return (make_token(scan, TOKEN_COLON_PLUS));
	else if (match(scan, '?'))
		return (make_token(scan, TOKEN_COLON_QUESTION));
	/* Lone ':' becomes a WORD (keeps behavior outside ${...}) */
	return (make_token(scan, TOKEN_WORD));
}

t_token handle_question(t_scanner *scan)
{
	advance(scan);
	if (match(scan, '-'))
		return (make_token(scan, TOKEN_COLON_DASH));
	else if (match(scan, '='))
		return (make_token(scan, TOKEN_COLON_EQUAL));
	else if (match(scan, '+'))
		return (make_token(scan, TOKEN_COLON_QUESTION));
	return (make_token(scan, TOKEN_ERR));
}

t_token handle_bang(t_scanner *scan)
{
	advance(scan);
	if (match(scan, '!'))
		return (make_token(scan, TOKEN_BANG_BANG));
	return (make_token(scan, TOKEN_BANG));
}

t_token handle_lbracket(t_scanner *scan)
{
	advance(scan);
	return (make_token(scan, TOKEN_GLOB_BRACKET_OPEN));
}

t_token handle_rbracket(t_scanner *scan)
{
	advance(scan);
	return (make_token(scan, TOKEN_GLOB_BRACKET_CLOSE));
}

/* dot handler */
t_token handle_dot(t_scanner *scan)
{
	advance(scan);
	/* fractional number like .5 */
	if (ft_isdigit(peek(scan)))
	{
		while (ft_isdigit(peek(scan)))
			advance(scan);
		return (make_token(scan, TOKEN_NUMBER));
	}
	/* If previous token was a glob (e.g., *.txt or file?.log), merge ".word" into IDENTIFIER */
	if ((scan->last_token == TOKEN_GLOB_STAR || scan->last_token == TOKEN_GLOB_QUESTION) && (ft_isalpha(peek(scan)) || peek(scan) == '_'))
	{
		/* start identifier after the dot */
		scan->start = scan->current;
		return (identifier(scan));
	}
	/* Otherwise, standalone dot */
	return (make_token(scan, TOKEN_DOT));
}

// ...existing code...
t_token handle_word(t_scanner *scan)
{
    const char *start_ptr = scan->current;
    char c;

    /* Consume a full word: stop only on whitespace or real metacharacters
       that change lexical state (pipes, redirects, newlines, quotes, $, etc.) */
    while (!scan_is_at_end(scan))
    {
        c = peek(scan);
        if (ft_isspace((int)c))
            break;
        if (c == '|' || c == '&' || c == ';' || c == '<' || c == '>' || c == '\n')
            break;
        if (c == '\'' || c == '"' || c == '`' || c == '\\' || c == '$' || c == '#')
            break;
        /* otherwise include character in the word (this keeps '/' and ':' etc.) */
        advance(scan);
    }

    /* ensure progress (avoid zero-length tokens) */
    if (scan->current == start_ptr)
        advance(scan);

    /* Decide token type after scanning the whole lexeme */
    int all_ident = 1;
    int all_digits = 1;
    for (const char *p = start_ptr; p < scan->current; ++p)
    {
        unsigned char ch = (unsigned char)*p;
        if (!(ft_isalnum(ch) || ch == '_'))
            all_ident = 0;
        if (!ft_isdigit(ch))
            all_digits = 0;
    }

    if (all_digits)
        return (make_token(scan, TOKEN_NUMBER));
    if (all_ident)
        return (make_token(scan, TOKEN_IDENTIFIER));
    return (make_token(scan, TOKEN_WORD));
}
// ...existing code...
// ...existing code...

t_token handle_whitespace(t_scanner *scan)
{
	while (ft_isspace(peek(scan)))
		advance(scan);
	return (make_token(scan, TOKEN_WHITESPACE));
}

t_token handle_eof(t_scanner *scan)
{
	return (make_token(scan, TOKEN_EOF));
}

t_token handle_literal(t_scanner *scan)
{
	char c;
	c = peek(scan);
	/* If it looks like an identifier, scan it */
	if (ft_isalpha(c) || c == '_')
		return (identifier(scan));
	/* If it looks like a number, scan it */
	if (ft_isdigit(c))
		return (number(scan));
	/* Otherwise consume a single character as a word/literal to avoid zero-length tokens */
	advance(scan);
	return (make_token(scan, TOKEN_WORD));
}
