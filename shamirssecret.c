/*
 * Shamir's secret sharing sharing implementation
 *
 * Copyright (C) 2013 Matt Corallo <git@bluematt.me>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307 USA.
 */

#include <stdint.h>

#ifndef IN_KERNEL
#include <assert.h>
#define CHECKSTATE(x) assert(x)
#else
#include <linux/bug.h>
#define CHECKSTATE(x) BUG_ON(x)
#endif

#include "shamirssecret.h"

/*
 * Calculations across the finite field GF(2^8)
 */

#ifndef TEST
static uint8_t field_add(uint8_t a, uint8_t b) {
	return a ^ b;
}

static uint8_t field_sub(uint8_t a, uint8_t b) {
	return a ^ b;
}

static uint8_t field_neg(uint8_t a) {
	return field_sub(0, a);
}
#endif

//TODO: Using static tables will very likely create side-channel attacks when measuring cache hits
//      Because these are fairly small tables, we can probably get them loaded mostly/fully into
//      cache before use to break such attacks.
static const uint8_t exp[P] = {
	0x01, 0x03, 0x05, 0x0f, 0x11, 0x33, 0x55, 0xff, 0x1a, 0x2e, 0x72, 0x96, 0xa1, 0xf8, 0x13, 0x35,
	0x5f, 0xe1, 0x38, 0x48, 0xd8, 0x73, 0x95, 0xa4, 0xf7, 0x02, 0x06, 0x0a, 0x1e, 0x22, 0x66, 0xaa,
	0xe5, 0x34, 0x5c, 0xe4, 0x37, 0x59, 0xeb, 0x26, 0x6a, 0xbe, 0xd9, 0x70, 0x90, 0xab, 0xe6, 0x31,
	0x53, 0xf5, 0x04, 0x0c, 0x14, 0x3c, 0x44, 0xcc, 0x4f, 0xd1, 0x68, 0xb8, 0xd3, 0x6e, 0xb2, 0xcd,
	0x4c, 0xd4, 0x67, 0xa9, 0xe0, 0x3b, 0x4d, 0xd7, 0x62, 0xa6, 0xf1, 0x08, 0x18, 0x28, 0x78, 0x88,
	0x83, 0x9e, 0xb9, 0xd0, 0x6b, 0xbd, 0xdc, 0x7f, 0x81, 0x98, 0xb3, 0xce, 0x49, 0xdb, 0x76, 0x9a,
	0xb5, 0xc4, 0x57, 0xf9, 0x10, 0x30, 0x50, 0xf0, 0x0b, 0x1d, 0x27, 0x69, 0xbb, 0xd6, 0x61, 0xa3,
	0xfe, 0x19, 0x2b, 0x7d, 0x87, 0x92, 0xad, 0xec, 0x2f, 0x71, 0x93, 0xae, 0xe9, 0x20, 0x60, 0xa0,
	0xfb, 0x16, 0x3a, 0x4e, 0xd2, 0x6d, 0xb7, 0xc2, 0x5d, 0xe7, 0x32, 0x56, 0xfa, 0x15, 0x3f, 0x41,
	0xc3, 0x5e, 0xe2, 0x3d, 0x47, 0xc9, 0x40, 0xc0, 0x5b, 0xed, 0x2c, 0x74, 0x9c, 0xbf, 0xda, 0x75,
	0x9f, 0xba, 0xd5, 0x64, 0xac, 0xef, 0x2a, 0x7e, 0x82, 0x9d, 0xbc, 0xdf, 0x7a, 0x8e, 0x89, 0x80,
	0x9b, 0xb6, 0xc1, 0x58, 0xe8, 0x23, 0x65, 0xaf, 0xea, 0x25, 0x6f, 0xb1, 0xc8, 0x43, 0xc5, 0x54,
	0xfc, 0x1f, 0x21, 0x63, 0xa5, 0xf4, 0x07, 0x09, 0x1b, 0x2d, 0x77, 0x99, 0xb0, 0xcb, 0x46, 0xca,
	0x45, 0xcf, 0x4a, 0xde, 0x79, 0x8b, 0x86, 0x91, 0xa8, 0xe3, 0x3e, 0x42, 0xc6, 0x51, 0xf3, 0x0e,
	0x12, 0x36, 0x5a, 0xee, 0x29, 0x7b, 0x8d, 0x8c, 0x8f, 0x8a, 0x85, 0x94, 0xa7, 0xf2, 0x0d, 0x17,
	0x39, 0x4b, 0xdd, 0x7c, 0x84, 0x97, 0xa2, 0xfd, 0x1c, 0x24, 0x6c, 0xb4, 0xc7, 0x52, 0xf6, 0x01};
static const uint8_t log[P] = {
	0x00, // log(0) is not defined
	0xff, 0x19, 0x01, 0x32, 0x02, 0x1a, 0xc6, 0x4b, 0xc7, 0x1b, 0x68, 0x33, 0xee, 0xdf, 0x03, 0x64,
	0x04, 0xe0, 0x0e, 0x34, 0x8d, 0x81, 0xef, 0x4c, 0x71, 0x08, 0xc8, 0xf8, 0x69, 0x1c, 0xc1, 0x7d,
	0xc2, 0x1d, 0xb5, 0xf9, 0xb9, 0x27, 0x6a, 0x4d, 0xe4, 0xa6, 0x72, 0x9a, 0xc9, 0x09, 0x78, 0x65,
	0x2f, 0x8a, 0x05, 0x21, 0x0f, 0xe1, 0x24, 0x12, 0xf0, 0x82, 0x45, 0x35, 0x93, 0xda, 0x8e, 0x96,
	0x8f, 0xdb, 0xbd, 0x36, 0xd0, 0xce, 0x94, 0x13, 0x5c, 0xd2, 0xf1, 0x40, 0x46, 0x83, 0x38, 0x66,
	0xdd, 0xfd, 0x30, 0xbf, 0x06, 0x8b, 0x62, 0xb3, 0x25, 0xe2, 0x98, 0x22, 0x88, 0x91, 0x10, 0x7e,
	0x6e, 0x48, 0xc3, 0xa3, 0xb6, 0x1e, 0x42, 0x3a, 0x6b, 0x28, 0x54, 0xfa, 0x85, 0x3d, 0xba, 0x2b,
	0x79, 0x0a, 0x15, 0x9b, 0x9f, 0x5e, 0xca, 0x4e, 0xd4, 0xac, 0xe5, 0xf3, 0x73, 0xa7, 0x57, 0xaf,
	0x58, 0xa8, 0x50, 0xf4, 0xea, 0xd6, 0x74, 0x4f, 0xae, 0xe9, 0xd5, 0xe7, 0xe6, 0xad, 0xe8, 0x2c,
	0xd7, 0x75, 0x7a, 0xeb, 0x16, 0x0b, 0xf5, 0x59, 0xcb, 0x5f, 0xb0, 0x9c, 0xa9, 0x51, 0xa0, 0x7f,
	0x0c, 0xf6, 0x6f, 0x17, 0xc4, 0x49, 0xec, 0xd8, 0x43, 0x1f, 0x2d, 0xa4, 0x76, 0x7b, 0xb7, 0xcc,
	0xbb, 0x3e, 0x5a, 0xfb, 0x60, 0xb1, 0x86, 0x3b, 0x52, 0xa1, 0x6c, 0xaa, 0x55, 0x29, 0x9d, 0x97,
	0xb2, 0x87, 0x90, 0x61, 0xbe, 0xdc, 0xfc, 0xbc, 0x95, 0xcf, 0xcd, 0x37, 0x3f, 0x5b, 0xd1, 0x53,
	0x39, 0x84, 0x3c, 0x41, 0xa2, 0x6d, 0x47, 0x14, 0x2a, 0x9e, 0x5d, 0x56, 0xf2, 0xd3, 0xab, 0x44,
	0x11, 0x92, 0xd9, 0x23, 0x20, 0x2e, 0x89, 0xb4, 0x7c, 0xb8, 0x26, 0x77, 0x99, 0xe3, 0xa5, 0x67,
	0x4a, 0xed, 0xde, 0xc5, 0x31, 0xfe, 0x18, 0x0d, 0x63, 0x8c, 0x80, 0xc0, 0xf7, 0x70, 0x07};

// We disable lots of optimizations that result in non-constant runtime (+/- branch delays)
static uint8_t field_mul_ret(uint8_t calc, uint8_t a, uint8_t b) __attribute__((optimize("-O0"))) __attribute__((noinline));
static uint8_t field_mul_ret(uint8_t calc, uint8_t a, uint8_t b) {
	uint8_t ret, ret2;
	if (a == 0)
		ret2 = 0;
	else
		ret2 = calc;
	if (b == 0)
		ret = 0;
	else
		ret = ret2;
	return ret;
}
static uint8_t field_mul(uint8_t a, uint8_t b)  {
	return field_mul_ret(exp[(log[a] + log[b]) % 255], a, b);
}

static uint8_t field_invert(uint8_t a) {
	CHECKSTATE(a != 0);
	return exp[0xff - log[a]]; // log[1] == 0xff
}

// We disable lots of optimizations that result in non-constant runtime (+/- branch delays)
static uint8_t field_pow_ret(uint8_t calc, uint8_t a, uint8_t e) __attribute__((optimize("-O0"))) __attribute__((noinline));
static uint8_t field_pow_ret(uint8_t calc, uint8_t a, uint8_t e) {
	uint8_t ret, ret2;
	if (a == 0)
		ret2 = 0;
	else
		ret2 = calc;
	if (e == 0)
		ret = 1;
	else
		ret = ret2;
	return ret;
}
static uint8_t field_pow(uint8_t a, uint8_t e) {
#ifndef TEST
	// Although this function works for a==0, its not trivially obvious why,
	// and since we never call with a==0, we just assert a != 0 (except when testing)
	CHECKSTATE(a != 0);
#endif
	return field_pow_ret(exp[(log[a] * e) % 255], a, e);
}

#ifdef TEST
static uint8_t field_mul_calc(uint8_t a, uint8_t b) {
	// side-channel attacks here
	uint8_t ret = 0;
	uint8_t counter;
	uint8_t carry;
	for (counter = 0; counter < 8; counter++) {
		if (b & 1)
			ret ^= a;
		carry = (a & 0x80);
		a <<= 1;
		if (carry)
			a ^= 0x1b; // what x^8 is modulo x^8 + x^4 + x^3 + x + 1
		b >>= 1;
	}
	return ret;
}
static uint8_t field_pow_calc(uint8_t a, uint8_t e) {
	uint8_t ret = 1;
	for (uint8_t i = 0; i < e; i++)
		ret = field_mul_calc(ret, a);
	return ret;
}
int main() {
	// Test inversion with the logarithm tables
	for (uint16_t i = 1; i < P; i++)
		CHECKSTATE(field_mul_calc(i, field_invert(i)) == 1);

	// Test multiplication with the logarithm tables
	for (uint16_t i = 0; i < P; i++) {
		for (uint16_t j = 0; j < P; j++)
			CHECKSTATE(field_mul(i, j) == field_mul_calc(i, j));
	}

	// Test exponentiation with the logarithm tables
	for (uint16_t i = 0; i < P; i++) {
		for (uint16_t j = 0; j < P; j++)
			CHECKSTATE(field_pow(i, j) == field_pow_calc(i, j));
	}
}
#endif // defined(TEST)



/*
 * Calculations across the polynomial q
 */
#ifndef TEST
/**
 * Calculates the Y coordinate that the point with the given X
 * coefficients[0] == secret, the rest are random values
 */
uint8_t calculateQ(uint8_t coefficients[], uint8_t shares_required, uint8_t x) {
	CHECKSTATE(x != 0); // q(0) == secret, though so does a[0]
	uint8_t ret = coefficients[0];
	for (uint8_t i = 1; i < shares_required; i++) {
		ret = field_add(ret, field_mul(coefficients[i], field_pow(x, i)));
	}
	return ret;
}

/**
 * Derives the secret given a set of shares_required points (x and q coordinates)
 */
uint8_t calculateSecret(uint8_t x[], uint8_t q[], uint8_t shares_required) {
	// Calculate the x^0 term using a derivation of the forumula at
	// http://en.wikipedia.org/wiki/Lagrange_polynomial#Example_2
	uint8_t ret = 0;
	for (uint8_t i = 0; i < shares_required; i++) {
		uint8_t temp = q[i];
		for (uint8_t j = 0; j < shares_required; j++) {
			if (i == j)
				continue;
			temp = field_mul(temp, field_neg(x[j]));
			temp = field_mul(temp, field_invert(field_sub(x[i], x[j])));
		}
		ret = field_add(ret, temp);
	}
	return ret;
}
#endif // !defined(TEST)
