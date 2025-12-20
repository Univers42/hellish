#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *ft_strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    char *new_s = malloc(len);
    if (!new_s)
        return (NULL);
    memcpy(new_s, s, len);
    return (new_s);
}

t_token *create_test_token(const char *value)
{
    t_token *tok = malloc(sizeof(t_token));
    if (!tok)
        return (NULL);
    (void)value; 
    if (0)
    {
        free(tok);
        return (NULL);
    }
    tok->type = TOKEN_WORD;
    return (tok);
}


void test_dynamic_strs()
{
    printf("--- Pruebas: str Dinámico ---\n");
    t_dynamic_str ds;
    int ok = 1;

    d_str_init(&ds, 10);
    if (ds.len != 0 || ds.dyn_str_capacity < 10 || !ds.str || ds.str[0] != '\0')
    {
        printf("  [FAIL] d_str_init no funciona como se esperaba.\n");
        ok = 0;
    }
    else
        printf("  [PASS] d_str_init funciona.\n");
    d_str_free(&ds);

    d_str_init(&ds, 2);
    d_str_append_char(&ds, 'a'); 
    d_str_append_char(&ds, 'b');
    d_str_append_char(&ds, 'c'); 
    if (ds.len != 3 || ds.dyn_str_capacity < 4 || strcmp(ds.str, "abc") != 0)
    {
         printf("  [FAIL] d_str_append_char (con redimensión) no funciona.\n");
         ok = 0;
    }
    else
        printf("  [PASS] d_str_append_char (con redimensión) funciona.\n");
    d_str_free(&ds);

    d_str_init(&ds, 4);
    d_str_append_str(&ds, "hola");
    d_str_append_str(&ds, " mundo");
    if (ds.len != 10 || ds.dyn_str_capacity < 11 || strcmp(ds.str, "hola mundo") != 0)
    {
        printf("  [FAIL] d_str_append_str (con redimensión) no funciona.\n");
        ok = 0;
    }
    else
        printf("  [PASS] d_str_append_str (con redimensión) funciona.\n");
    
    d_str_append_char(&ds, '!');
     if (ds.len != 11 || strcmp(ds.str, "hola mundo!") != 0)
    {
        printf("  [FAIL] d_str_append_char (tras redimensión) no funciona.\n");
        ok = 0;
    }
     else
        printf("  [PASS] d_str_append_char (tras redimensión) funciona.\n");
    d_str_free(&ds);

    // Test for d_str_reset
    d_str_init(&ds, 20);
    d_str_append_str(&ds, "string to reset");
    d_str_reset(&ds);
    if (ds.len != 0 || ds.str[0] != '\0' || ds.dyn_str_capacity < 20)
    {
        printf("  [FAIL] d_str_reset no limpia el string correctamente.\n");
        ok = 0;
    }
    else
        printf("  [PASS] d_str_reset funciona.\n");

    // Verify reuse after reset
    d_str_append_str(&ds, "new data");
    if (ds.len != 8 || strcmp(ds.str, "new data") != 0)
    {
        printf("  [FAIL] La reutilización del buffer después de d_str_reset ha fallado.\n");
        ok = 0;
    }
    else
        printf("  [PASS] La reutilización del buffer después de d_str_reset funciona.\n");
    d_str_free(&ds);

    if (ok)
        printf("\n  Resumen: Todas las pruebas de str dinámico pasaron.\n\n");
    else
         printf("\n  Resumen: ¡Fallaron algunas pruebas de str dinámico!\n\n");
}


void test_vector_init()
{
    printf("--- Pruebas: Vector Dinámico ---\n");
    t_token_vector *vec = vector_init(10);

    if (vec != NULL)
        printf("  [PASS] El vector no es NULL.\n");
    else
    {
        printf("  [FAIL] El vector es NULL. No se pueden continuar las pruebas.\n");
        return;
    }
    
    if (vec->tokens != NULL)
        printf("  [PASS] El array de tokens no es NULL.\n");
    else
        printf("  [FAIL] El array de tokens es NULL.\n");

    if (vec->count == 0)
        printf("  [PASS] El contador (count) es 0.\n");
    else
        printf("  [FAIL] El contador (count) es %zu en lugar de 0.\n", vec->count);

    if (vec->capacity == 10)
        printf("  [PASS] La capacidad (capacity) es 10.\n");
    else
        printf("  [FAIL] La capacidad (capacity) es %zu en lugar de 10.\n", vec->capacity);
    
    vector_free(vec);
    printf("  [INFO] vector_free ejecutado sin fallos en vector vacío.\n\n");
}

void test_lexer_simple_words()
{
    printf("--- Pruebas: Lexer (Palabras Simples) ---\n");
    const char *input = "  echo   hello world  ";
    t_token_vector *vec = lexer(input);
    int ok = 1;

    if (vec == NULL)
    {
        printf("  [FAIL] El lexer ha devuelto NULL.\n");
        ok = 0;
    }
    else if (vec->count != 3)
    {
        printf("  [FAIL] El lexer no ha encontrado el número correcto de tokens (esperado: 3, obtenido: %zu).\n", vec->count);
        ok = 0;
    }
    else
    {
        printf("  [PASS] El lexer ha encontrado 3 tokens.\n");
        if (strcmp(vec->tokens[0]->value.str, "echo") != 0 || vec->tokens[0]->type != TOKEN_WORD)
        {
            printf("  [FAIL] Token 1 incorrecto (esperado: 'echo', WORD).\n");
            ok = 0;
        }
        else
             printf("  [PASS] Token 1 es correcto ('echo').\n");
        if (strcmp(vec->tokens[1]->value.str, "hello") != 0 || vec->tokens[1]->type != TOKEN_WORD)
        {
            printf("  [FAIL] Token 2 incorrecto (esperado: 'hello', WORD).\n");
            ok = 0;
        }
        else
             printf("  [PASS] Token 2 es correcto ('hello').\n");
        if (strcmp(vec->tokens[2]->value.str, "world") != 0 || vec->tokens[2]->type != TOKEN_WORD)
        {
            printf("  [FAIL] Token 3 incorrecto (esperado: 'world', WORD).\n");
            ok = 0;
        }
        else
             printf("  [PASS] Token 3 es correcto ('world').\n");
    }

    if (ok)
        printf("\n  Resumen: Todas las pruebas del lexer simple pasaron.\n\n");
    else
        printf("\n  Resumen: ¡Fallaron algunas pruebas del lexer simple!\n\n");

    vector_free(vec);
}


int main(void)
{
    printf("=========================================\n");
    printf("  INICIANDO PRUEBAS DE LEXER (CORE)\n");
    printf("=========================================\n\n");

    test_dynamic_strs();
    test_vector_init();
    test_lexer_simple_words();

    printf("=========================================\n");
    printf("  FIN DE LAS PRUEBAS\n");
    printf("=========================================\n");

    return (0);
}
