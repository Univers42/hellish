#include <stdio.h>
#include <locale.h>
#include <wchar.h>

int main() {
    setlocale(LC_ALL, "");
    printf("width 🧑: %d\n", wcwidth(L'🧑'));
    printf("width 📂: %d\n", wcwidth(L'📂'));
    printf("width : %d\n", wcwidth(L''));
    printf("width 🐍: %d\n", wcwidth(L'🐍'));
    printf("width 🕰: %d\n", wcwidth(L'🕰'));
    printf("width ️ (fe0f): %d\n", wcwidth(0xFE0F));
    return 0;
}
