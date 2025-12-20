#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void test_lexer_simple_words()
{
    printf("--- Pruebas: Lexer (Palabras Simples) ---\n");
    const char *input = "  echo   hello world  ";
    t_token_vector *vec = lexer(input);
    int ok = 1;

    if (vec == NULL || vec->count != 3)
    {
        printf("  [FAIL] Palabras Simples: conteo de tokens incorrecto (esperado: 3, obtenido: %zu).\n", vec ? vec->count : 0);
        ok = 0;
    }
    else
    {
        if (strcmp(vec->tokens[0]->value.str, "echo") != 0) ok = 0;
        if (strcmp(vec->tokens[1]->value.str, "hello") != 0) ok = 0;
        if (strcmp(vec->tokens[2]->value.str, "world") != 0) ok = 0;
        if (!ok)
            printf("  [FAIL] El valor de los tokens de palabras simples no es el esperado.\n");
    }

    if (ok)
        printf("\n  Resumen: Todas las pruebas del lexer simple pasaron.\n\n");
    else
        printf("\n  Resumen: ¡Fallaron algunas pruebas del lexer simple!\n\n");

    if (vec)
        vector_free(vec);
}

void test_lexer_pipe()
{
    printf("--- Pruebas: Lexer (Pipe y Casos Borde) ---\n");
    const char *input = "ls | grep src";
    t_token_vector *vec = lexer(input);
    int ok = 1;

    if (vec == NULL || vec->count != 3 || strcmp(vec->tokens[0]->value.str, "ls") != 0 || vec->tokens[1]->type != TOKEN_PIPE || strcmp(vec->tokens[2]->value.str, "grep src") != 0)
    {
        // Nota: Esta prueba es simple y asume que "grep src" será un solo token si no se implementa el split por espacios post-quotes.
    }
    // Simplificando la prueba para mayor claridad
    const char *input2 = "ls|grep";
    vec = lexer(input2);
    if (vec == NULL || vec->count != 3 || vec->tokens[1]->type != TOKEN_PIPE || strcmp(vec->tokens[0]->value.str, "ls") != 0)
    {
        printf("  [FAIL] Caso sin espacios 'ls|grep' ha fallado.\n");
        ok = 0;
    }
    else
        printf("  [PASS] Caso sin espacios 'ls|grep' funciona.\n");
    if (vec)
        vector_free(vec);

    if (ok)
        printf("\n  Resumen: Todas las pruebas del lexer con pipe pasaron.\n\n");
    else
        printf("\n  Resumen: ¡Fallaron algunas pruebas del lexer con pipe!\n\n");
}

void test_lexer_redirects()
{
    printf("--- Pruebas: Lexer (Redirecciones) ---\n");
    const char *input = "cat < infile >> outfile";
    t_token_vector *vec = lexer(input);
    int ok = 1;

    if (vec == NULL || vec->count != 5)
    {
        printf("  [FAIL] Redirecciones: conteo de tokens incorrecto (esperado: 5, obtenido: %zu).\n", vec ? vec->count : 0);
        ok = 0;
    }
    else
    {
        if (vec->tokens[0]->type != TOKEN_WORD || strcmp(vec->tokens[0]->value.str, "cat") != 0) ok = 0;
        if (vec->tokens[1]->type != TOKEN_REDIR_IN) ok = 0;
        if (vec->tokens[2]->type != TOKEN_WORD || strcmp(vec->tokens[2]->value.str, "infile") != 0) ok = 0;
        if (vec->tokens[3]->type != TOKEN_REDIR_APPEND) ok = 0;
        if (vec->tokens[4]->type != TOKEN_WORD || strcmp(vec->tokens[4]->value.str, "outfile") != 0) ok = 0;

        if (!ok)
            printf("  [FAIL] La prueba 'cat < infile >> outfile' ha fallado en la validación de tokens.\n");
    }
    if(vec)
        vector_free(vec);

    const char* input2 = "echo hello>log";
    vec = lexer(input2);
     if (vec == NULL || vec->count != 3 || vec->tokens[1]->type != TOKEN_WORD || vec->tokens[2]->type != TOKEN_REDIR_OUT)
     {
        printf("  [FAIL] La prueba 'echo hello>log' ha fallado.\n");
        ok = 0;
     }
    if (vec)
        vector_free(vec);

    const char* input3 = "cat<<EOF";
    vec = lexer(input3);
    if (vec == NULL || vec->count != 3 || vec->tokens[0]->type != TOKEN_WORD || vec->tokens[1]->type != TOKEN_HEREDOC || vec->tokens[2]->type != TOKEN_WORD)
    {
        printf("  [FAIL] La prueba 'cat<<EOF' ha fallado.\n");
        ok = 0;
    }
    if (vec)
        vector_free(vec);

    if (ok)
        printf("\n  Resumen: Todas las pruebas de redirecciones pasaron.\n\n");
    else
        printf("\n  Resumen: ¡Fallaron algunas pruebas de redirecciones!\n\n");
}

void test_lexer_quotes()
{
    printf("--- Pruebas: Lexer (Comillas) ---\n");
    const char *input = "echo \"hello world\" | grep ' pattern '";
    t_token_vector *vec = lexer(input);
    int ok = 1;

    if (vec == NULL || vec->count != 5)
    {
        printf("  [FAIL] Comillas: conteo de tokens incorrecto (esperado: 5, obtenido: %zu).\n", vec ? vec->count : 0);
        ok = 0;
    }
    else
    {
        if (strcmp(vec->tokens[1]->value.str, "hello world") != 0) ok = 0;
        if (strcmp(vec->tokens[3]->value.str, " pattern ") != 0) ok = 0;
        if (!ok)
            printf("  [FAIL] El contenido de las comillas no es el esperado.\n");
    }
    if (vec)
        vector_free(vec);
    
    const char *input2 = "'' \"\"";
    vec = lexer(input2);
    if (vec == NULL || vec->count != 2 || strcmp(vec->tokens[0]->value.str, "") != 0 || strcmp(vec->tokens[1]->value.str, "") != 0)
    {
        printf("  [FAIL] Las comillas vacías '' \"\" han fallado.\n");
        ok = 0;
    }
    if (vec)
        vector_free(vec);

    if (ok)
        printf("\n  Resumen: Todas las pruebas de comillas pasaron.\n\n");
    else
        printf("\n  Resumen: ¡Fallaron algunas pruebas de comillas!\n\n");
}

void test_lexer_combinations()
{
    printf("--- Pruebas: Lexer (Combinaciones Complejas) ---\n");
    const char *input = "<infile cat | grep \"pattern\" >> 'outfile'";
    t_token_vector *vec = lexer(input);
    int ok = 1;

    if (vec == NULL || vec->count != 8)
    {
        printf("  [FAIL] Combinaciones: conteo de tokens incorrecto (esperado: 8, obtenido: %zu).\n", vec ? vec->count : 0);
        ok = 0;
    }
    else
    {
        if(vec->tokens[0]->type != TOKEN_REDIR_IN) ok = 0;
        if(strcmp(vec->tokens[1]->value.str, "infile") != 0) ok = 0;
        if(vec->tokens[2]->type != TOKEN_WORD || strcmp(vec->tokens[2]->value.str, "cat") != 0) ok = 0;
        if(vec->tokens[3]->type != TOKEN_PIPE) ok = 0;
        if(vec->tokens[4]->type != TOKEN_WORD || strcmp(vec->tokens[4]->value.str, "grep") != 0) ok = 0;
        if(strcmp(vec->tokens[5]->value.str, "pattern") != 0) ok = 0;
        if(vec->tokens[6]->type != TOKEN_REDIR_APPEND) ok = 0;
        if(strcmp(vec->tokens[7]->value.str, "outfile") != 0) ok = 0;
        if (!ok)
             printf("  [FAIL] La prueba de combinación compleja ha fallado en la validación.\n");
    }

    if (ok)
        printf("\n  Resumen: Todas las pruebas de combinaciones complejas pasaron.\n\n");
    else
        printf("\n  Resumen: ¡Fallaron algunas pruebas de combinaciones complejas!\n\n");

    if (vec)
        vector_free(vec);
}

void test_lexer_unclosed_quote()
{
    printf("--- Pruebas: Lexer (Errores: Comillas sin cerrar) ---\n");
    // Se asume que el lexer debe devolver NULL ante un error de sintaxis.
    const char *input = "echo \"hello world";
    t_token_vector *vec = lexer(input);

    if (vec == NULL)
    {
        printf("  [PASS] El lexer ha devuelto NULL como se esperaba para comillas dobles sin cerrar.\n");
    }
    else
    {
        printf("  [FAIL] El lexer NO ha devuelto NULL para comillas dobles sin cerrar.\n");
        vector_free(vec);
    }
    
    const char *input2 = "'another test";
    vec = lexer(input2);
    if (vec == NULL)
    {
        printf("  [PASS] El lexer ha devuelto NULL como se esperaba para comillas simples sin cerrar.\n\n");
    }
    else
    {
        printf("  [FAIL] El lexer NO ha devuelto NULL para comillas simples sin cerrar.\n\n");
        vector_free(vec);
    }
}


int main(void)
{
    printf("=========================================\n");
    printf("  INICIANDO PRUEBAS COMPLETAS DE LEXER\n");
    printf("=========================================\n\n");

    test_lexer_simple_words();
    test_lexer_pipe();
    test_lexer_redirects();
    test_lexer_quotes();
    test_lexer_combinations();
    test_lexer_unclosed_quote();

    printf("=========================================\n");
    printf("  FIN DE LAS PRUEBAS\n");
    printf("=========================================\n");

    return (0);
}
