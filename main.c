/*
 * Shamir's secret sharing CLI interface
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

#include "shamirssecret.h"

#define MAX_LENGTH 1024
#define ERROREXIT(str...) {fprintf(stderr, str); exit(1);}

#ifndef RAND_SOURCE
#define RAND_SOURCE "/dev/random"
#endif

#ifndef TEST
static void derive_missing_part(uint8_t total_shares, uint8_t shares_required, bool parts_have[], const uint8_t* split_version, const uint8_t* split_x, uint8_t split_index, uint8_t split_size) {
	const uint8_t (*D)[split_size] = (const uint8_t (*)[split_size])split_version;
	uint8_t x[shares_required], q[shares_required];

	// Fill in x/q with the selected shares
	uint16_t x_pos = 0;
	for (uint8_t i = 0; i < P-1; i++) {
		if (parts_have[i]) {
			x[x_pos] = split_x[i];
			q[x_pos++] = D[i][split_index];
		}
	}
	assert(x_pos == shares_required - 1);

	// Now loop through ALL x we didn't already set (despite not having that many
	// shares, because more shares could be added arbitrarily, any x should not be
	// able to rule out any possible secrets) and try each possible q, making sure
	// that each q gives us a new possibility for the secret.
	bool impossible_secrets[P];
	memset(impossible_secrets, 0, sizeof(impossible_secrets));
	for (uint16_t final_x = 1; final_x < P; final_x++) { 
		bool x_already_used = false;
		for (uint8_t j = 0; j < shares_required; j++) {
			if (x[j] == final_x)
				x_already_used = true;
		}
		if (x_already_used)
			continue;

		x[shares_required-1] = final_x;
		bool possible_secrets[P];
		memset(possible_secrets, 0, sizeof(possible_secrets));
		for (uint16_t new_q = 0; new_q < P; new_q++) {
			q[shares_required-1] = new_q;
			possible_secrets[calculateSecret(x, q, shares_required)] = 1;
		}

		for (uint16_t i = 0; i < P; i++)
			assert(possible_secrets[i]);
	}

	//TODO: Check that gcc isn't optimizing this one away
	memset(q, 0, sizeof(q));
}

static void check_possible_missing_part_derivations_intern(uint8_t total_shares, uint8_t shares_required, bool parts_have[], uint8_t parts_included, uint16_t progress, const uint8_t* split_version, const uint8_t* x, uint8_t split_index, uint8_t split_size) {
	if (parts_included == shares_required-1)
		return derive_missing_part(total_shares, shares_required, parts_have, split_version, x, split_index, split_size);

	if (total_shares - progress < shares_required)
		return;

	check_possible_missing_part_derivations_intern(total_shares, shares_required, parts_have, parts_included, progress+1, split_version, x, split_index, split_size);
	parts_have[progress] = 1;
	check_possible_missing_part_derivations_intern(total_shares, shares_required, parts_have, parts_included+1, progress+1, split_version, x, split_index, split_size);
	parts_have[progress] = 0;
}

static void check_possible_missing_part_derivations(uint8_t total_shares, uint8_t shares_required, const uint8_t* split_version, const uint8_t* x, uint8_t split_index, uint8_t split_size) {
	bool parts_have[P];
	memset(parts_have, 0, sizeof(parts_have));
	check_possible_missing_part_derivations_intern(total_shares, shares_required, parts_have, 0, 0, split_version, x, split_index, split_size);
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

		FILE* random = fopen(RAND_SOURCE, "r");
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

		uint8_t a[shares_required], x[total_shares], D[total_shares][secret_length];

		// TODO: The following loop may take a long time and eat lots of /dev/random if total_shares is high
		for (uint32_t i = 0; i < total_shares; i++) {
			int32_t j = 0;
			do {
				assert(fread(&x[i], sizeof(uint8_t), 1, random) == 1);
				if (x[i] == 0)
					continue;
				for (j = 0; j < i; j++)
					if (x[j] == x[i])
						break;
			} while (j < i); // Inner loop will get to j = i when x[j] != x[i] for all j
			if (i % 32 == 31)
				printf("Finished picking X coordinates for %u shares\n", i+1);
		}
		for (uint32_t i = 0; i < secret_length; i++) {
			a[0] = secret[i];

			for (uint8_t j = 1; j < shares_required; j++)
				assert(fread(&a[j], sizeof(uint8_t), 1, random) == 1);
			for (uint8_t j = 0; j < total_shares; j++)
				D[j][i] = calculateQ(a, shares_required, x[j]);

			// Now, for paranoia's sake, we ensure that no matter which piece we are missing, we can derive no information about the secret
			check_possible_missing_part_derivations(total_shares, shares_required, &(D[0][0]), x, i, secret_length);

			if (i % 32 == 31)
				printf("Finished processing %u bytes.\n", i+1);
		}

		char out_file_name_buf[strlen(out_file_param) + 4];
		strcpy(out_file_name_buf, out_file_param);
		for (uint8_t i = 0; i < total_shares; i++) {
			sprintf(((char*)out_file_name_buf) + strlen(out_file_param), "%u", i);
			FILE* out_file = fopen(out_file_name_buf, "w+");
			if (!out_file)
				ERROREXIT("Could not open output file %s\n", out_file_name_buf)

			if (fwrite(&x[i], sizeof(uint8_t), 1, out_file) != 1)
				ERROREXIT("Could not write 1 byte to %s\n", out_file_name_buf)

			if (fwrite(D[i], 1, secret_length, out_file) != secret_length)
				ERROREXIT("Could not write %lu bytes to %s\n", secret_length, out_file_name_buf)

			fclose(out_file);
		}

		// Clear sensitive data (No, GCC 4.7.2 is currently not optimizing this out)
		memset(secret, 0, sizeof(uint8_t)*secret_length);
		memset(a, 0, sizeof(uint8_t)*shares_required);
		memset(x, 0, sizeof(uint8_t)*total_shares);
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
