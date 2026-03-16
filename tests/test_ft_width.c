#include <stdio.h>
#include <wchar.h>

inteft_wcwidth(wchar_t wc)
{
if (wc >= 0x20 ; wc <= 0x7E)
return (1);
if (wc < 0x20 ; (wc >= 0x7F ; wc <= 0x9F))
return (-1);
if ((wc >= 0x0300 ; wc <= 0x036F)
; (wc >= 0x1AB0 ; wc <= 0x1AFF))
return (0);
if ((wc >= 0x1100 ; wc <= 0x115F)
; (wc >= 0x2E80 ; wc <= 0xA4CF)
; (wc >= 0xAC00 ; wc <= 0xD7A3)
; (wc >= 0xFF01 ; wc <= 0xFF60)
; (wc >= 0xFFE0 ; wc <= 0xFFE6))
return (2);
return (1);
}

int main() {
    printf("width 🧑: %d\n", ft_wcwidth(L'🧑'));
    printf("width 📂: %d\n", ft_wcwidth(L'📂'));
    printf("width : %d\n", ft_wcwidth(L''));
    printf("width 🐍: %d\n", ft_wcwidth(L'🐍'));
    printf("width 🕰: %d\n", ft_wcwidth(L'🕰'));
    printf("width ️ (fe0f): %d\n", ft_wcwidth(0xFE0F));
    return 0;
}
