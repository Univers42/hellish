#include "lexer.h"

void    handle_redirect(t_lexer_state *state)
{
    finalize_word(state);
    if (state->input[*state->i] == '>')
    {
        d_str_append_char(state->current_token_str, state->input[*state->i]);
        (*state->i)++;
        if (state->input[*state->i] == '>')
        {
            d_str_append_char(state->current_token_str, state->input[*state->i]);
            (*state->i)++;
            add_token_to_vector(state->vector, state->current_token_str, TOKEN_REDIR_APPEND);
        }
        else
            add_token_to_vector(state->vector, state->current_token_str, TOKEN_REDIR_OUT);
    }
    else if (state->input[*state->i] == '<')
    {
        d_str_append_char(state->current_token_str, state->input[*state->i]);
        (*state->i)++;
        if (state->input[*state->i] == '<')
        {
            d_str_append_char(state->current_token_str, state->input[*state->i]);
            (*state->i)++;
            add_token_to_vector(state->vector, state->current_token_str, TOKEN_HEREDOC);
        }
        else
            add_token_to_vector(state->vector, state->current_token_str, TOKEN_REDIR_IN);
    }
}
