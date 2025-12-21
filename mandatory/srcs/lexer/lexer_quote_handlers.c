#include "lexer.h"

int handle_quotes(t_lexer_state *state)
{
    char    quote;
    
    quote = state->input[*state->i];
    (*state->i)++;
    while (state->input[*state->i] && state->input[*state->i] != quote)
    {
        d_str_append_char(state->current_token_str, state->input[*state->i]);
        (*state->i)++;
    }
    if (state->input[*state->i] == quote)
        (*state->i)++;
    else
        return (0);
    add_token_to_vector(state->vector, state->current_token_str, TOKEN_WORD);
    return (1);
}
