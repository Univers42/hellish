#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Estructura para definir un token esperado en las pruebas
typedef struct {
    const char* value;
    t_tok_type type;
} t_expected_token;

// Helper para convertir el tipo de token a string para logs más claros
const char *token_type_to_str(t_tok_type type)
{
    switch (type)
    {
        case TOK_WORD: return "WORD";
        case TOK_PIPE: return "PIPE";
        case TOK_REDIR_IN: return "REDIR_IN";
        case TOK_REDIR_OUT: return "REDIR_OUT";
        case TOK_HEREDOC: return "HEREDOC";
        case TOK_REDIR_APPEND: return "APPEND";
        default: return "UNKNOWN";
    }
}

void print_table_header(const char *input)
{
    printf("--- Test: '%s' ---\n", input);
    printf("| %-3s | %-15s | %-20s | %-15s | %-20s | %-6s |\n", "Idx", "Expected Type", "Expected Value", "Obtained Type", "Obtained Value", "Status");
    printf("|-----|-----------------|----------------------|-----------------|----------------------|--------|\n");
}

void print_test_result_row(int i, t_expected_token *expected, t_tok *obtained, int is_ok)
{
    char expected_type_str[20] = "N/A";
    char expected_value_str[21] = "N/A";
    char obtained_type_str[20] = "N/A";
    char obtained_value_str[21] = "N/A";

    if (expected)
    {
        snprintf(expected_type_str, 20, "%s", token_type_to_str(expected->type));
        snprintf(expected_value_str, 21, "%.20s", expected->value);
    }
    if (obtained)
    {
        snprintf(obtained_type_str, 20, "%s", token_type_to_str(obtained->type));
        snprintf(obtained_value_str, 21, "%.20s", obtained->value.str);
    }
    printf("| %-3d | %-15s | %-20s | %-15s | %-20s | %-6s |\n", i, expected_type_str, expected_value_str, obtained_type_str, obtained_value_str, is_ok ? "PASS" : "FAIL");
}


int run_test(const char *input, t_expected_token expected[], int expected_count)
{
    print_table_header(input);
    t_tok_vec *vec = lexer(input);
    int overall_ok = 1;

    // Caso especial para esperar un error del lexer (ej: comillas sin cerrar)
    if (expected_count < 0)
    {
        if (vec == NULL) {
            printf("| Result: Lexer returned NULL as expected for invalid input.\n");
            printf("\nResultado Final: PASS\n\n");
            return 1;
        } else {
            printf("| ERROR: Lexer did NOT return NULL for invalid input.\n");
            printf("\nResultado Final: FAIL\n\n");
            vector_free(vec);
            return 0;
        }
    }

    if (vec == NULL)
    {
        printf("| ERROR: Lexer returned NULL unexpectedly.\n");
		printf("\nResultado Final: FAIL\n\n");
        return 0;
    }

    if (vec->count != (size_t)expected_count)
    {
        printf("| FAIL: Token count mismatch. Expected: %d, Got: %zu\n", expected_count, vec->count);
        overall_ok = 0;
    }

    size_t max_tokens = vec->count > (size_t)expected_count ? vec->count : (size_t)expected_count;
    for (size_t i = 0; i < max_tokens; i++)
    {
        t_expected_token *p_expected = (i < (size_t)expected_count) ? &expected[i] : NULL;
        t_tok *p_obtained = (i < vec->count) ? vec->tokens[i] : NULL;
        int row_ok = 1;

        if (!p_expected || !p_obtained || p_obtained->type != p_expected->type || strcmp(p_obtained->value.str, p_expected->value) != 0) {
            row_ok = 0;
            overall_ok = 0;
        }
        print_test_result_row((int)i, p_expected, p_obtained, row_ok);
    }
    
    printf("\nResultado Final: %s\n\n", overall_ok ? "PASS" : "FAIL");
    vector_free(vec);
    return overall_ok;
}

int main(void)
{
    printf("=========================================\n");
    printf("  INICIANDO BATERÍA DE PRUEBAS DEL LEXER\n");
    printf("=========================================\n\n");

    // --- GRUPO 1: Casos Básicos y Espacios ---
    printf("--- GRUPO 1: Casos Básicos y Espacios ---\n\n");
    t_expected_token test1[] = {{"echo", TOK_WORD}, {"hello", TOK_WORD}};
    run_test("echo hello", test1, 2);
    
    t_expected_token test2[] = {{"ls", TOK_WORD}, {"-la", TOK_WORD}};
    run_test("   ls    -la   ", test2, 2);
    
    run_test("", NULL, 0);
    run_test("   \t\n   ", NULL, 0);

    // --- GRUPO 2: Operadores (Pipes y Redirecciones) ---
    printf("--- GRUPO 2: Operadores (Pipes y Redirecciones) ---\n\n");
    t_expected_token test3[] = {{"ls", TOK_WORD}, {"|", TOK_PIPE}, {"grep", TOK_WORD}, {"main", TOK_WORD}};
    run_test("ls|grep main", test3, 4);
    
    t_expected_token test4[] = {{"ls", TOK_WORD}, {"|", TOK_PIPE}, {"grep", TOK_WORD}, {"|", TOK_PIPE}, {"wc", TOK_WORD}};
    run_test("ls | grep | wc", test4, 5);

    t_expected_token test5[] = {{"echo", TOK_WORD}, {"hello", TOK_WORD}, {">", TOK_REDIR_OUT}, {"log", TOK_WORD}};
    run_test("echo hello>log", test5, 4);

    t_expected_token test6[] = {{"cat", TOK_WORD}, {"<", TOK_REDIR_IN}, {"f1", TOK_WORD}, {">>", TOK_REDIR_APPEND}, {"f2", TOK_WORD}};
    run_test("cat<f1 >> f2", test6, 5);

    t_expected_token test7[] = {{"cat", TOK_WORD}, {"<<", TOK_HEREDOC}, {"EOF", TOK_WORD}};
    run_test("cat<<EOF", test7, 3);
    
    t_expected_token test_ops[] = {{">", TOK_REDIR_OUT}, {"<", TOK_REDIR_IN}, {"|", TOK_PIPE}, {">>", TOK_REDIR_APPEND}, {"HOLA", TOK_WORD}};
    run_test("><|>>HOLA", test_ops, 5);


    // --- GRUPO 3: Comillas y Casos Complejos ---
    printf("--- GRUPO 3: Comillas y Casos Complejos ---\n\n");
    t_expected_token test8[] = {{"echo", TOK_WORD}, {"hello world", TOK_WORD}};
    run_test("echo \"hello world\"", test8, 2);

    t_expected_token test9[] = {{"grep", TOK_WORD}, {"  spaced pattern  ", TOK_WORD}};
    run_test("grep '  spaced pattern  '", test9, 2);
    
    t_expected_token test10[] = {{"", TOK_WORD}};
    run_test("\"\"", test10, 1);

    t_expected_token test11[] = {{"", TOK_WORD}};
    run_test("''", test11, 1);

    t_expected_token test12[] = {{"echo", TOK_WORD}, {"'hello'world", TOK_WORD}};
    run_test("echo \"'hello'world\"", test12, 2);

    t_expected_token test13[] = {{"echo", TOK_WORD}, {"", TOK_WORD}, {"", TOK_WORD}};
    run_test("echo '' \"\"", test13, 3);


    // --- GRUPO 4: Combinaciones y Casos Borde ---
    printf("--- GRUPO 4: Combinaciones y Casos Borde ---\n\n");
    t_expected_token test14[] = {{"<", TOK_REDIR_IN}, {"f", TOK_WORD}, {"cmd", TOK_WORD}, {"-arg", TOK_WORD}, {"pat tern", TOK_WORD}, {"|", TOK_PIPE}, {"cmd2", TOK_WORD}, {">>", TOK_REDIR_APPEND}, {"out", TOK_WORD}};
    run_test("<f cmd -arg 'pat tern' | cmd2 >> out", test14, 9);
    
    t_expected_token test15[] = {{"echo", TOK_WORD}, {"a", TOK_WORD}, {">", TOK_REDIR_OUT}, {"b", TOK_WORD}, {"|", TOK_PIPE}, {"grep", TOK_WORD}, {"c", TOK_WORD}};
    run_test("echo a>b|grep c", test15, 7);

    // --- GRUPO 5: Errores (Entradas Inválidas) ---
    printf("--- GRUPO 5: Errores (Entradas Inválidas) ---\n\n");
    run_test("echo \"hello", NULL, -1);
    run_test("echo 'world", NULL, -1);
    run_test("echo 'mismatched\"", NULL, -1);


    printf("=========================================\n");
    printf("  FIN DE LA BATERÍA DE PRUEBAS\n");
    printf("=========================================\n");

    return (0);
}
