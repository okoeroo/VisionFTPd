#include "irandom.h"


char random_lowercase (void)
{
    return (random() % ('z' - 'a' + 1)) + 'a';
}
char random_uppercase (void)
{
    return (random() % ('Z' - 'A' + 1)) + 'A';
}
char random_digit (void)
{
    return (random() % ('9' - '0' + 1)) + '0';
}
char random_chars (void)
{
    return ((random() % 2) ? random_uppercase() : random_lowercase());
}
char random_alphanum (void)
{
    switch (random() % 3)
    {
        case 0 :
            return random_uppercase();
        case 1 :
            return random_lowercase();
        case 2 :
            return random_digit();
        default:
            return random_digit();
    }
}
