#include "../libft.h"
#include <stdlib.h>
#include <string.h>

char    *ft_strndup(const char *s, size_t n)
{
    char    *dup;
    size_t  i;

    dup = (char *)malloc(sizeof(char) * (n + 1));
    if (!dup)
        return (NULL);
    i = 0;
    while (i < n && s[i])
    {
        dup[i] = s[i];
        i++;
    }
    dup[i] = '\0';
    return (dup);
}