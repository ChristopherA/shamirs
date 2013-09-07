#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>

#define MAX_LENGTH 1024
#define ERROREXIT(str...) {fprintf(stderr, str); exit(1);}

/*
 * Calculations across the finite field GF(2^8)
 */
#define P 256

static uint8_t field_add(uint8_t a, uint8_t b) {
	return a ^ b;
}

static uint8_t field_sub(uint8_t a, uint8_t b) {
	return a ^ b;
}

static uint8_t field_neg(uint8_t a) {
	return field_sub(0, a);
}

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
	assert(a != 0);
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
	assert(a != 0);
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
		assert(field_mul_calc(i, field_invert(i)) == 1);

	// Test multiplication with the logarithm tables
	for (uint16_t i = 0; i < P; i++) {
		for (uint16_t j = 0; j < P; j++)
			assert(field_mul(i, j) == field_mul_calc(i, j));
	}

	// Test exponentiation with the logarithm tables
	for (uint16_t i = 0; i < P; i++) {
		for (uint16_t j = 0; j < P; j++)
			assert(field_pow(i, j) == field_pow_calc(i, j));
	}
}
#endif // defined(TEST)



/*
 * Calculations across the polynomial q
 */
#ifndef TEST
static uint8_t calculateQ(uint8_t a[], uint8_t k, uint8_t x) {
	assert(x != 0); // q(0) == secret, though so does a[0]
	uint8_t ret = a[0];
	for (uint8_t i = 1; i < k; i++) {
		ret = field_add(ret, field_mul(a[i], field_pow(x, i)));
	}
	return ret;
}

uint8_t calculateSecret(uint8_t x[], uint8_t q[], uint8_t k) {
	// Calculate the x^0 term using a derivation of the forumula at
	// http://en.wikipedia.org/wiki/Lagrange_polynomial#Example_2
	uint8_t ret = 0;
	for (uint8_t i = 0; i < k; i++) {
		uint8_t temp = q[i];
		for (uint8_t j = 0; j < k; j++) {
			if (i == j)
				continue;
			temp = field_mul(temp, field_neg(x[j]));
			temp = field_mul(temp, field_invert(field_sub(x[i], x[j])));
		}
		ret = field_add(ret, temp);
	}
	return ret;
}



int main(int argc, char* argv[]) {
	assert(mlockall(MCL_CURRENT | MCL_FUTURE) == 0);

	char split = 0;
	uint8_t total_shares = 0, shares_required = 0;
	char* files[P]; uint8_t files_count = 0;
	char *in_file = (void*)0, *out_file_param = (void*)0;

	int i;
	while((i = getopt(argc, argv, "scn:k:f:o:i:h?")) != -1)
		switch(i) {
		case 's':
			if ((split & 0x2) && !(split & 0x1))
				ERROREXIT("-s (split) and -c (combine) are mutually exclusive\n")
			else
				split = (0x2 | 0x1);
			break;
		case 'c':
			if ((split & 0x2) && (split & 0x1))
				ERROREXIT("-s (split) and -c (combine) are mutually exclusive\n")
			else
				split = 0x2;
			break;
		case 'n': {
			int t = atoi(optarg);
			if (t <= 0 || t >= P)
				ERROREXIT("n must be > 0 and < %u\n", P)
			else
				total_shares = t;
			break;
		}
		case 'k': {
			int t = atoi(optarg);
			if (t <= 0 || t >= P)
				ERROREXIT("n must be > 0 and < %u\n", P)
			else
				shares_required = t;
			break;
		}
		case 'i':
			in_file = optarg;
			break;
		case 'o':
			out_file_param = optarg;
			break;
		case 'f':
			if (files_count >= P-1)
				ERROREXIT("May only specify up to %u files\n", P-1)
			files[files_count++] = optarg;
			break;
		case 'h':
		case '?':
			printf("Split usage: -s -n <total shares> -k <shares required> -i <input file> -o <output file path base>\n");
			printf("Combine usage: -c -k <shares provided == shares required> <-f <share>>*k -o <output file>\n");
			exit(0);
			break;
		default:
			ERROREXIT("getopt failed?\n")
		}
	if (!(split & 0x2))
		ERROREXIT("Must specify one of -c, -s or -?\n")
	split &= 0x1;

	if (argc != optind)
		ERROREXIT("Invalid argument\n")

	if (split) {
		if (!total_shares || !shares_required)
			ERROREXIT("n and k must be set.\n")

		if (shares_required > total_shares)
			ERROREXIT("k must be <= n\n")

		if (files_count != 0 || !in_file || !out_file_param)
			ERROREXIT("Must specify -i <input file> and -o <output file path base> but not -f in split mode.\n")

		FILE* random = fopen("/dev/random", "r");
		assert(random);
		FILE* secret_file = fopen(in_file, "r");
		if (!secret_file)
			ERROREXIT("Could not open %s for reading.\n", in_file)

		uint8_t secret[MAX_LENGTH];

		size_t secret_length = fread(secret, 1, MAX_LENGTH*sizeof(uint8_t), secret_file);
		if (secret_length == 0)
			ERROREXIT("Error reading secret\n")
		if (fread(secret, 1, 1, secret_file) > 0)
			ERROREXIT("Secret may not be longer than %u\n", MAX_LENGTH)
		fclose(secret_file);
		printf("Using secret of length %lu\n", secret_length);

		uint8_t a[shares_required], D[total_shares][secret_length];

		for (uint32_t i = 0; i < secret_length; i++) {
			a[0] = secret[i];

			for (uint8_t j = 1; j < shares_required; j++)
				assert(fread(&a[j], sizeof(uint8_t), 1, random) == 1);
			for (uint8_t j = 0; j < total_shares; j++)
				D[j][i] = calculateQ(a, shares_required, j+1);

			if (i % 32 == 0 && i != 0)
				printf("Finished processing %u bytes.\n", i);
		}

		char out_file_name_buf[strlen(out_file_param) + 4];
		strcpy(out_file_name_buf, out_file_param);
		for (uint8_t i = 0; i < total_shares; i++) {
			/*printf("%u-", i);
			for (uint8_t j = 0; j < secret_length; j++)
				printf("%02x", D[i][j]);
			printf("\n");*/

			sprintf(((char*)out_file_name_buf) + strlen(out_file_param), "%u", i);
			FILE* out_file = fopen(out_file_name_buf, "w+");
			if (!out_file)
				ERROREXIT("Could not open output file %s\n", out_file_name_buf)

			uint8_t x = i+1;
			if (fwrite(&x, sizeof(uint8_t), 1, out_file) != 1)
				ERROREXIT("Could not write 1 byte to %s\n", out_file_name_buf)

			if (fwrite(D[i], 1, secret_length, out_file) != secret_length)
				ERROREXIT("Could not write %lu bytes to %s\n", secret_length, out_file_name_buf)

			fclose(out_file);
		}
		/*printf("secret = ");
		for (uint8_t i = 0; i < secret_length; i++)
			printf("%02x", secret[i]);
		printf("\n");*/

		// Clear sensitive data (No, GCC 4.7.2 is currently not optimizing this out)
		memset(secret, 0, sizeof(uint8_t)*secret_length);
		memset(a, 0, sizeof(uint8_t)*shares_required);
		memset(in_file, 0, strlen(in_file));

		fclose(random);
	} else {
		if (!shares_required)
			ERROREXIT("k must be set.\n")

		if (files_count != shares_required || in_file || !out_file_param)
			ERROREXIT("Must not specify -i and must specify -o and exactly k -f <input file>s in combine mode.\n")

		uint8_t x[shares_required], q[shares_required];
		FILE* files_fps[shares_required];

		for (uint8_t i = 0; i < shares_required; i++) {
			files_fps[i] = fopen(files[i], "r");
			if (!files_fps[i])
				ERROREXIT("Couldn't open file %s for reading.\n", files[i])
			if (fread(&x[i], sizeof(uint8_t), 1, files_fps[i]) != 1)
				ERROREXIT("Couldn't read the x byte of %s\n", files[i])
		}

		uint8_t secret[MAX_LENGTH];

		uint32_t i = 0;
		while (fread(&q[0], sizeof(uint8_t), 1, files_fps[0]) == 1) {
			for (uint8_t j = 1; j < shares_required; j++) {
				if (fread(&q[j], sizeof(uint8_t), 1, files_fps[j]) != 1)
					ERROREXIT("Couldn't read next byte from %s\n", files[j])
			}
			secret[i++] = calculateSecret(x, q, shares_required);
		}
		printf("Got secret of length %u\n", i);

		FILE* out_file = fopen(out_file_param, "w+");
		fwrite(secret, sizeof(uint8_t), i, out_file);
		fclose(out_file);

		for (uint8_t i = 0; i < shares_required; i++)
			fclose(files_fps[i]);

		// Clear sensitive data (No, GCC 4.7.2 is currently not optimizing this out)
		memset(secret, 0, sizeof(uint8_t)*i);
		memset(q, 0, sizeof(uint8_t)*shares_required);
		memset(out_file_param, 0, strlen(out_file_param));
		for (uint8_t i = 0; i < shares_required; i++)
			memset(files[i], 0, strlen(files[i]));
		memset(x, 0, sizeof(uint8_t)*shares_required);
	}

	return 0;
}
#endif // !defined(TEST)
