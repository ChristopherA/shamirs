#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>

#define MAX_LENGTH 1024

/*
 * Calculations across the finite field GF(p)
 * Lots of side-channel attacks in here
 */
/*
const uint8_t p = 251; // Largest 8-bit prime
static uint8_t field_add(uint8_t a, uint8_t b) {
	assert(a < p && b < p);
	return (((uint16_t)a) + ((uint16_t)b)) % p;
}

static uint8_t field_sub(uint8_t a, uint8_t b) {
	assert(a < p && b < p);
	return (((uint16_t)p) + ((uint16_t)a) - ((uint16_t)b)) % p;
}

static uint8_t field_mul(uint8_t a, uint8_t b) {
	assert(a < p && b < p);
	return (((uint16_t)a) * ((uint16_t)b)) % p;
}

static uint8_t field_pow(uint8_t a, uint8_t e) {
	assert(a < p);
	uint8_t ret = 1;
	for (uint8_t i = 0; i < e; i++) {
		ret = field_mul(ret, a);
	}
	return ret;
}

static uint8_t field_neg(uint8_t a) {
	assert(a < p);
	if (a == 0)
		return 0;
	return p - a;
}

static uint8_t field_invert(uint8_t a) {
	// Brute force, yay!
	assert(a < p);
	for (uint8_t i = 0; i < p; i++) {
		if (field_mul(i, a) == 1)
			return i;
	}
	assert(0);
}*/



/*
 * Calculations across the finite field GF(2^8)
 */
const uint16_t p = 256;
static uint8_t field_add(uint8_t a, uint8_t b) {
	return a ^ b;
}

static uint8_t field_sub(uint8_t a, uint8_t b) {
	return a ^ b;
}

static uint8_t field_mul(uint8_t a, uint8_t b) {
	// TODO side-channel attacks here?
	uint8_t ret = 0;
	uint8_t counter;
	uint8_t carry;
	for (counter = 0; counter < 8; counter++) {
		if (b & 1)
			ret ^= a;
		carry = (a & 0x80);
		a <<= 1;
		if (carry)
			a ^= 0x1b; /* what x^8 is modulo x^8 + x^4 + x^3 + x + 1 */
		b >>= 1;
	}
	return ret;
}

// WARNING: Do not use if e is secret (potential side-channel attacks)
static uint8_t field_pow(uint8_t a, uint8_t e) {
	// TODO: This could be sped up pretty trivially
	uint8_t ret = 1;
	for (uint8_t i = 0; i < e; i++)
		ret = field_mul(ret, a);
	return ret;
}

static uint8_t field_neg(uint8_t a) {
	return field_sub(0, a);
}

static const uint8_t inverse[] = { // Multiplicative inverse of each element in the field
	0xff, // 0 has no inverse
	0x01, 0x8d, 0xf6, 0xcb, 0x52, 0x7b, 0xd1, 0xe8, 0x4f, 0x29, 0xc0, 0xb0, 0xe1, 0xe5, 0xc7, 0x74,
	0xb4, 0xaa, 0x4b, 0x99, 0x2b, 0x60, 0x5f, 0x58, 0x3f, 0xfd, 0xcc, 0xff, 0x40, 0xee, 0xb2, 0x3a,
	0x6e, 0x5a, 0xf1, 0x55, 0x4d, 0xa8, 0xc9, 0xc1, 0x0a, 0x98, 0x15, 0x30, 0x44, 0xa2, 0xc2, 0x2c,
	0x45, 0x92, 0x6c, 0xf3, 0x39, 0x66, 0x42, 0xf2, 0x35, 0x20, 0x6f, 0x77, 0xbb, 0x59, 0x19, 0x1d,
	0xfe, 0x37, 0x67, 0x2d, 0x31, 0xf5, 0x69, 0xa7, 0x64, 0xab, 0x13, 0x54, 0x25, 0xe9, 0x09, 0xed,
	0x5c, 0x05, 0xca, 0x4c, 0x24, 0x87, 0xbf, 0x18, 0x3e, 0x22, 0xf0, 0x51, 0xec, 0x61, 0x17, 0x16,
	0x5e, 0xaf, 0xd3, 0x49, 0xa6, 0x36, 0x43, 0xf4, 0x47, 0x91, 0xdf, 0x33, 0x93, 0x21, 0x3b, 0x79,
	0xb7, 0x97, 0x85, 0x10, 0xb5, 0xba, 0x3c, 0xb6, 0x70, 0xd0, 0x06, 0xa1, 0xfa, 0x81, 0x82, 0x83,
	0x7e, 0x7f, 0x80, 0x96, 0x73, 0xbe, 0x56, 0x9b, 0x9e, 0x95, 0xd9, 0xf7, 0x02, 0xb9, 0xa4, 0xde,
	0x6a, 0x32, 0x6d, 0xd8, 0x8a, 0x84, 0x72, 0x2a, 0x14, 0x9f, 0x88, 0xf9, 0xdc, 0x89, 0x9a, 0xfb,
	0x7c, 0x2e, 0xc3, 0x8f, 0xb8, 0x65, 0x48, 0x26, 0xc8, 0x12, 0x4a, 0xce, 0xe7, 0xd2, 0x62, 0x0c,
	0xe0, 0x1f, 0xef, 0x11, 0x75, 0x78, 0x71, 0xa5, 0x8e, 0x76, 0x3d, 0xbd, 0xbc, 0x86, 0x57, 0x0b,
	0x28, 0x2f, 0xa3, 0xda, 0xd4, 0xe4, 0x0f, 0xa9, 0x27, 0x53, 0x04, 0x1b, 0xfc, 0xac, 0xe6, 0x7a,
	0x07, 0xae, 0x63, 0xc5, 0xdb, 0xe2, 0xea, 0x94, 0x8b, 0xc4, 0xd5, 0x9d, 0xf8, 0x90, 0x6b, 0xb1,
	0x0d, 0xd6, 0xeb, 0xc6, 0x0e, 0xcf, 0xad, 0x08, 0x4e, 0xd7, 0xe3, 0x5d, 0x50, 0x1e, 0xb3, 0x5b,
	0x23, 0x38, 0x34, 0x68, 0x46, 0x03, 0x8c, 0xdd, 0x9c, 0x7d, 0xa0, 0xcd, 0x1a, 0x41, 0x1c};
static uint8_t field_invert(uint8_t a) {
	assert(a != 0);
	return inverse[a];
}



/*
 * Calculations across the polynomial q
 */
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

#define ERROREXIT(str...) {fprintf(stderr, str); exit(1);}

int main(int argc, char* argv[]) {
	assert(mlockall(MCL_CURRENT | MCL_FUTURE) == 0);

	char split = 0;
	uint8_t n = 0, k = 0;
	char* files[p]; uint8_t files_count = 0;
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
			if (t <= 0 || t >= p)
				ERROREXIT("n must be > 0 and < %u\n", p)
			else
				n = t;
			break;
		}
		case 'k': {
			int t = atoi(optarg);
			if (t <= 0 || t >= p)
				ERROREXIT("n must be > 0 and < %u\n", p)
			else
				k = t;
			break;
		}
		case 'i':
			in_file = optarg;
			break;
		case 'o':
			out_file_param = optarg;
			break;
		case 'f':
			if (files_count >= p-1)
				ERROREXIT("May only specify up to %u files\n", p-1)
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
		ERROREXIT("Must specify either -c or -s\n")
	split &= 0x1;

	if (argc != optind)
		ERROREXIT("Invalid argument\n")

	if (split) {
		if (!n || !k)
			ERROREXIT("n and k must be set.\n")

		if (k > n)
			ERROREXIT("k must be <= n\n")

		if (files_count != 0 || !in_file || !out_file_param)
			ERROREXIT("Must specify -i <input file> and -o <output file path base> but not -f in split mode.\n")

		FILE* random = fopen("/dev/random", "r");
		assert(random);
		FILE* secret_file = fopen(in_file, "r");
		if (!secret_file)
			ERROREXIT("Could not open %s for reading.\n", in_file)

		uint8_t secret[MAX_LENGTH];
		memset(secret, 0, MAX_LENGTH*sizeof(uint8_t));

		size_t secret_length = fread(secret, 1, MAX_LENGTH*sizeof(uint8_t), secret_file);
		if (secret_length == 0)
			ERROREXIT("Error reading secret\n")
		if (fread(secret, 1, 1, secret_file) > 0)
			ERROREXIT("Secret may not be longer than %u\n", MAX_LENGTH)
		fclose(secret_file);
		printf("Using secret of length %lu\n", secret_length);

		uint8_t a[secret_length][k], D[n][secret_length];

		for (uint8_t i = 0; i < secret_length; i++) {
			a[i][0] = secret[i];

			for (uint8_t j = 1; j < k; j++) {
				do
					assert(fread(&a[i][j], sizeof(uint8_t), 1, random) == 1);
				while (a[i][j] >= p);
			}
			for (uint8_t j = 0; j < n; j++)
				D[j][i] = calculateQ(a[i], k, j+1);
		}

		char out_file_name_buf[strlen(out_file_param) + 4];
		strcpy(out_file_name_buf, out_file_param);
		for (uint8_t i = 0; i < n; i++) {
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

		fclose(random);
	} else {
		if (!k)
			ERROREXIT("k must be set.\n")

		if (files_count != k || in_file || !out_file_param)
			ERROREXIT("Must not specify -i and must specify -o and exactly k -f <input file>s in combine mode.\n")

		uint8_t x[k], q[k];
		FILE* files_fps[k];

		for (uint8_t i = 0; i < k; i++) {
			files_fps[i] = fopen(files[i], "r");
			if (!files_fps[i])
				ERROREXIT("Couldn't open file %s for reading.\n", files[i])
			if (fread(&x[i], sizeof(uint8_t), 1, files_fps[i]) != 1)
				ERROREXIT("Couldn't read the x byte of %s\n", files[i])
		}

		uint8_t secret[MAX_LENGTH];

		uint8_t i = 0;
		while (fread(&q[0], sizeof(uint8_t), 1, files_fps[0]) == 1) {
			for (uint8_t j = 1; j < k; j++) {
				if (fread(&q[j], sizeof(uint8_t), 1, files_fps[j]) != 1)
					ERROREXIT("Couldn't read next byte from %s\n", files[j])
			}
			secret[i++] = calculateSecret(x, q, k);
		}
		printf("Got secret of length %u\n", i);

		FILE* out_file = fopen(out_file_param, "w+");
		fwrite(secret, sizeof(uint8_t), i, out_file);
		fclose(out_file);

		for (uint8_t i = 0; i < k; i++)
			fclose(files_fps[i]);
	}

	return 0;
}
