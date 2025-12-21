#include "lexer.h"

void    finalize_word(t_lexer_state *state)
{
    if (state->current_token_str->len > 0)
        add_token_to_vector(state->vector, state->current_token_str, TOKEN_WORD);
}

void    handle_pipe(t_lexer_state *state)
{
    finalize_word(state);
    d_str_append_char(state->current_token_str, state->input[*state->i]);
    (*state->i)++;
    add_token_to_vector(state->vector, state->current_token_str, TOKEN_PIPE);
}

void    handle_whitespace(t_lexer_state *state)
{
    finalize_word(state);
    while (state->input[*state->i] && is_whitespace(state->input[*state->i]))
        (*state->i)++;
}
