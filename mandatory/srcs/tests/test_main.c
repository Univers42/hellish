#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Estructura para definir un token esperado en las pruebas
typedef struct {
    const char* value;
    t_token_type type;
} t_expected_token;

// Helper para convertir el tipo de token a string para logs más claros
const char *token_type_to_str(t_token_type type)
{
    switch (type)
    {
        case TOKEN_WORD: return "WORD";
        case TOKEN_PIPE: return "PIPE";
        case TOKEN_REDIR_IN: return "REDIR_IN";
        case TOKEN_REDIR_OUT: return "REDIR_OUT";
        case TOKEN_HEREDOC: return "HEREDOC";
        case TOKEN_REDIR_APPEND: return "APPEND";
        default: return "UNKNOWN";
    }
}

void print_table_header(const char *input)
{
    printf("--- Test: '%s' ---\n", input);
    printf("| %-3s | %-15s | %-20s | %-15s | %-20s | %-6s |\n", "Idx", "Expected Type", "Expected Value", "Obtained Type", "Obtained Value", "Status");
    printf("|-----|-----------------|----------------------|-----------------|----------------------|--------|\n");
}

void print_test_result_row(int i, t_expected_token *expected, t_token *obtained, int is_ok)
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
    t_token_vector *vec = lexer(input);
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
        t_token *p_obtained = (i < vec->count) ? vec->tokens[i] : NULL;
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
    t_expected_token test1[] = {{"echo", TOKEN_WORD}, {"hello", TOKEN_WORD}};
    run_test("echo hello", test1, 2);
    
    t_expected_token test2[] = {{"ls", TOKEN_WORD}, {"-la", TOKEN_WORD}};
    run_test("   ls    -la   ", test2, 2);
    
    run_test("", NULL, 0);
    run_test("   \t\n   ", NULL, 0);

    // --- GRUPO 2: Operadores (Pipes y Redirecciones) ---
    printf("--- GRUPO 2: Operadores (Pipes y Redirecciones) ---\n\n");
    t_expected_token test3[] = {{"ls", TOKEN_WORD}, {"|", TOKEN_PIPE}, {"grep", TOKEN_WORD}, {"main", TOKEN_WORD}};
    run_test("ls|grep main", test3, 4);
    
    t_expected_token test4[] = {{"ls", TOKEN_WORD}, {"|", TOKEN_PIPE}, {"grep", TOKEN_WORD}, {"|", TOKEN_PIPE}, {"wc", TOKEN_WORD}};
    run_test("ls | grep | wc", test4, 5);

    t_expected_token test5[] = {{"echo", TOKEN_WORD}, {"hello", TOKEN_WORD}, {">", TOKEN_REDIR_OUT}, {"log", TOKEN_WORD}};
    run_test("echo hello>log", test5, 4);

    t_expected_token test6[] = {{"cat", TOKEN_WORD}, {"<", TOKEN_REDIR_IN}, {"f1", TOKEN_WORD}, {">>", TOKEN_REDIR_APPEND}, {"f2", TOKEN_WORD}};
    run_test("cat<f1 >> f2", test6, 5);

    t_expected_token test7[] = {{"cat", TOKEN_WORD}, {"<<", TOKEN_HEREDOC}, {"EOF", TOKEN_WORD}};
    run_test("cat<<EOF", test7, 3);
    
    t_expected_token test_ops[] = {{">", TOKEN_REDIR_OUT}, {"<", TOKEN_REDIR_IN}, {"|", TOKEN_PIPE}, {">>", TOKEN_REDIR_APPEND}, {"HOLA", TOKEN_WORD}};
    run_test("><|>>HOLA", test_ops, 5);


    // --- GRUPO 3: Comillas y Casos Complejos ---
    printf("--- GRUPO 3: Comillas y Casos Complejos ---\n\n");
    t_expected_token test8[] = {{"echo", TOKEN_WORD}, {"hello world", TOKEN_WORD}};
    run_test("echo \"hello world\"", test8, 2);

    t_expected_token test9[] = {{"grep", TOKEN_WORD}, {"  spaced pattern  ", TOKEN_WORD}};
    run_test("grep '  spaced pattern  '", test9, 2);
    
    t_expected_token test10[] = {{"", TOKEN_WORD}};
    run_test("\"\"", test10, 1);

    t_expected_token test11[] = {{"", TOKEN_WORD}};
    run_test("''", test11, 1);

    t_expected_token test12[] = {{"echo", TOKEN_WORD}, {"'hello'world", TOKEN_WORD}};
    run_test("echo \"'hello'world\"", test12, 2);

    t_expected_token test13[] = {{"echo", TOKEN_WORD}, {"", TOKEN_WORD}, {"", TOKEN_WORD}};
    run_test("echo '' \"\"", test13, 3);


    // --- GRUPO 4: Combinaciones y Casos Borde ---
    printf("--- GRUPO 4: Combinaciones y Casos Borde ---\n\n");
    t_expected_token test14[] = {{"<", TOKEN_REDIR_IN}, {"f", TOKEN_WORD}, {"cmd", TOKEN_WORD}, {"-arg", TOKEN_WORD}, {"pat tern", TOKEN_WORD}, {"|", TOKEN_PIPE}, {"cmd2", TOKEN_WORD}, {">>", TOKEN_REDIR_APPEND}, {"out", TOKEN_WORD}};
    run_test("<f cmd -arg 'pat tern' | cmd2 >> out", test14, 9);
    
    t_expected_token test15[] = {{"echo", TOKEN_WORD}, {"a", TOKEN_WORD}, {">", TOKEN_REDIR_OUT}, {"b", TOKEN_WORD}, {"|", TOKEN_PIPE}, {"grep", TOKEN_WORD}, {"c", TOKEN_WORD}};
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
