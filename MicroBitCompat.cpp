/**
  * Compatibility / portability funcitons and constants for the MicroBit DAL.
  */
#include "MicroBit.h"

/**
  * Performs an in buffer reverse of a given char array
  */
void string_reverse(char *s) 
{
    char *j;
    int c;
 
    j = s + strlen(s) - 1;
    
    while(s < j) 
    {
        c = *s;
        *s++ = *j;
        *j-- = c;
    }
}

/**
  * Converts a given integer into a base 10 ASCII equivalent.
  *
  * @param n The number to convert.
  * @param s Pointer to a buffer in which to store the resulting string.
  */
void itoa(int n, char *s)
{
    int i = 0;
    int sign = (n > 0);

    // Record the sign of the number,
    // Ensure our working value is positive.
    if (!sign)  
        n = -n;          

    // Calculate each character, starting with the LSB.    
    do {       
         s[i++] = n % 10 + '0';   
    } while ((n /= 10) > 0);     
    
    // Add a negative sign as needed
    if (!sign)
        s[i++] = '-';
    
    // Terminate the string.
    s[i] = '\0';
    
    // Flip the order.
    string_reverse(s);
}