#include "../libft.h"

/*
 * ft_atol:
 *  文字列をlongに変換し返す関数。
 *  (シンプルな実装例。要件に応じてエラーチェックなどを強化。)
 */
long ft_atol(const char *str)
{
    long result;
    long sign;
    int  i;

    result = 0;
    sign = 1;
    i = 0;
    while ((str[i] >= 9 && str[i] <= 13) || str[i] == ' ')
        i++;
    if (str[i] == '-' || str[i] == '+')
    {
        if (str[i] == '-')
            sign = -1;
        i++;
    }
    while (str[i] && (str[i] >= '0' && str[i] <= '9'))
    {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return (result * sign);
}