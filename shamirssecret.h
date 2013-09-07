#include <stdint.h>

#define P 256

/**
 * Calculates the Y coordinate that the point with the given X
 * coefficients[0] == secret, the rest are random values
 */
uint8_t calculateQ(uint8_t coefficients[], uint8_t shares_required, uint8_t x);

/**
 * Derives the secret given a set of shares_required points (x and q coordinates)
 */
uint8_t calculateSecret(uint8_t x[], uint8_t q[], uint8_t shares_required);
