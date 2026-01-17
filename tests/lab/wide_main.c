#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <string.h>

// int main(void)
// {
//     setlocale(LC_ALL, "");
    
//     printf("Testing character widths:\n");
//     printf("╭ (U+256D) = %d\n", wcwidth(L'╭'));
//     printf("╰ (U+2570) = %d\n", wcwidth(L'╰'));
//     printf("─ (U+2500) = %d\n", wcwidth(L'─'));
//     printf("● (U+25CF) = %d\n", wcwidth(L'●'));
//     printf("⏳ (U+23F3) = %d\n", wcwidth(L'⏳'));
//     printf("❯ (U+276F) = %d\n", wcwidth(L'❯'));
//     printf("➜ (U+279C) = %d\n", wcwidth(L'➜'));
    
//     printf("\nActual display test (should align):\n");
//     printf("ASCII:   |XX|\n");
//     printf("╰─:      |╰─|\n");
//     printf("If these don't align, ╰─ is NOT width-2 in your terminal!\n");
    
//     printf("\nCounting ╰─ together:\n");
//     mbstate_t state;
//     memset(&state, 0, sizeof(state));
//     const char *str = "╰─";
//     int total = 0;
//     const char *p = str;
//     while (*p) {
//         wchar_t wc;
//         size_t len = mbrtowc(&wc, p, 10, &state);
//         if (len == (size_t)-1 || len == (size_t)-2 || len == 0) break;
//         int w = wcwidth(wc);
//         printf("  char at offset %ld: width=%d\n", p - str, w);
//         total += w;
//         p += len;
//     }
//     printf("Total width of ╰─: %d\n", total);
    
//     return 0;
// }