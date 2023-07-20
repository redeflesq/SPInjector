#include "tointeger.h"

int to_integer(char* s)
{
    int c = 1, a = 0, sign, start, end, base = 1;
    //Determine if the number is negative or positive 
    if (s[0] == '-')
        sign = -1;
    else if (s[0] <= '9' && s[0] >= '0')
        sign = 1;
    else if (s[0] == '+')
        sign = 2;
    //No further processing if it starts with a letter 
    else
        return 0;
    //Scanning the string to find the position of the last consecutive number
    while (s[c] != '\n' && s[c] <= '9' && s[c] >= '0')
        c++;
    //Index of the last consecutive number from beginning
    start = c - 1;
    //Based on sign, index of the 1st number is set
    if (sign == -1)
        end = 1;
    else if (sign == 1)
        end = 0;
    //When it starts with +, it is actually positive but with a different index 
    //for the 1st number
    else
    {
        end = 1;
        sign = 1;
    }
    //This the main loop of algorithm which generates the absolute value of the 
    //number from consecutive numerical characters.  
    for (int i = start; i >= end; i--)
    {
        a += (s[i] - '0') * base;
        base *= 10;
    }
    //The correct sign of generated absolute value is applied
    return sign * a;
}