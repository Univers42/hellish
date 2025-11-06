/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_types.c                                       :+:      :+:    :+:   */
/*                                                                            */
/* ************************************************************************** */

#include "arena.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <locale.h>

int main(void)
{
	t_stack_mark    mark;
	int             *pint;
	unsigned int    *puint;
	long            *plong;
	float           *pfloat;
	double          *pdouble;
	char            *pchar;
	bool			*pbool;
	uint8_t			*pu8;
	uint16_t		*pu16;
	uint32_t		*pu32;
	uint64_t		*pu64;
	size_t			*psize;
	ptrdiff_t		*pptrdiff;

	setlocale(LC_ALL, "C");
	printf("==== libmemalloc Type Compatibility Test ====\n");
	set_stack_mark(&mark);
	pint = (int *)st_alloc(sizeof(int));
	*pint = 42;
	printf("\nint:           %d\n", *pint);
	puint = (unsigned int *)st_alloc(sizeof(unsigned int));
	*puint = 4294967295U;
	printf("unsigned int:  %u\n", *puint);
	plong = (long *)st_alloc(sizeof(long));
	*plong = 9223372036854775807L;
	printf("long:          %ld\n", *plong);
	pfloat = (float *)st_alloc(sizeof(float));
	*pfloat = 3.14159f;
	printf("float:         %f\n", *pfloat);
	pdouble = (double *)st_alloc(sizeof(double));
	*pdouble = 2.718281828459045;
	printf("double:        %g\n", *pdouble);
	pchar = (char *)st_alloc(sizeof(char));
	*pchar = 'X';
	printf("char:          %c\n", *pchar);
	pbool = (bool *)st_alloc(sizeof(bool));
	*pbool = true;
	printf("bool:          %s\n", *pbool ? "true" : "false");
	pu8 = (uint8_t *)st_alloc(sizeof(uint8_t));
	*pu8 = 255;
	printf("uint8_t:       %u\n", *pu8);
	pu16 = (uint16_t *)st_alloc(sizeof(uint16_t));
	*pu16 = 65535;
	printf("uint16_t:      %u\n", *pu16);
	pu32 = (uint32_t *)st_alloc(sizeof(uint32_t));
	*pu32 = 4294967295U;
	printf("uint32_t:      %u\n", *pu32);
	pu64 = (uint64_t *)st_alloc(sizeof(uint64_t));
	*pu64 = 18446744073709551615UL;
	printf("uint64_t:      %lu\n", *pu64);
	psize = (size_t *)st_alloc(sizeof(size_t));
	*psize = 1024;
	printf("size_t:        %zu\n", *psize);
	pptrdiff = (ptrdiff_t *)st_alloc(sizeof(ptrdiff_t));
	*pptrdiff = -42;
	printf("ptrdiff_t:     %td\n", *pptrdiff);
	printf("\nAll types allocated successfully (arena is type-agnostic)\n");
	pop_stack_mark(&mark);
	arena_cleanup();
	fflush(NULL);
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
	return (0);
}
