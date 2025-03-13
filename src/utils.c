#include "utils.h"

unsigned int hex_to_uint(char *hex_str, unsigned int len) {
    /**
     * hex_to_uint - Converts a hexadecimal string to an unsigned integer
     * @str: Pointer to a character array containing the hexadecimal string
     * @len: Number of characters to process from the string
     *
     * Example: "1A3" would be converted to decimal value 419
     *
     * Return: The unsigned integer value of the hexadecimal string
     */
    unsigned int result = 0;
    while (len--) {
        result = result << 4;
        if (*hex_str >= '0' && *hex_str <= '9') {
            result += *hex_str - '0';
        }
        else if (*hex_str >= 'a' && *hex_str <= 'f') {
            result += *hex_str - 'a' + 10;
        }
        else if (*hex_str >= 'A' && *hex_str <= 'F') {
            result += *hex_str - 'A' + 10;
        }
        hex_str++;
    }
    return result;
}

int atoi(char *str) {
    /**
     * atoi - Converts a string to an integer
     * @str: Pointer to a character array containing the string
     *
     * Example: "123" would be converted to decimal value 123
     *
     * Return: The integer value of the string
     */
    int result = 0;
    while (*str) {
        result = result * 10 + *str - '0';
        str++;
    }
    return result;
}

unsigned int align(unsigned int n, unsigned int alignment) {
    return (n + alignment - 1) & ~(alignment - 1);
}