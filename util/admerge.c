/*
 * Copyright (c) 2025 saybur
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * Quick hack to overwrite portions of an AppleDouble file with entries
 * from another one. This is intended to take FinderData and resource
 * forks from a dot file (._) and place them inside a default
 * AppleDouble file created by Netatalk.
 *
 * This reads both files into memory and re-writes the output from that
 * stored state. This is more flexible and more dangerous than just
 * re-writing the FinderData and appending the resource fork in-place,
 * which would probably be good enough based on what I've seen of
 * Netatalk default files. Oh well. A rewrite may happen if there are
 * breaking changes.
 *
 * As noted this is a *hack* and there are probably serious bugs in
 * the code. Use with GREAT CAUTION. Suggestions, comments, and
 * improvements are all welcome.
 */

// limit on the number of descriptors allowed
#define MAX_ENTRIES 32
// arbitrary limit on desciptor data size
#define MAX_ENTRY_SIZE 131072

typedef struct {
	uint32_t id;
	uint32_t offset; // ephemeral
	uint32_t length;
	uint8_t *data;
} ad_entry;

// controls sort order within the header
// here to support Netatalk placing 0x8053567E after 0x8053594E
static uint32_t adouble_order_header(uint32_t id)
{
	switch (id) {
		case 0x8053567E:
			return 0x8053594F;
		default:
			return id;
	}
}

// controls sort order within the data section; resource fork needs to
// be at the end, the others are set based on observation of files
static uint32_t adouble_order_data(uint32_t id)
{
	switch (id) {
		case 2:
			return 4294967295;
		case 15:
			return 11;
		case 14:
			return 12;
		case 12:
			return 14;
		case 11:
			return 15;
		case 0x8053567E:
			return 0x8053594F;
		default:
			return id;
	}
}

// provides base size for certain elements that need to have a fixed
// allocation size within the file; these are derived from observation
// of existing files
static uint32_t adouble_size(uint32_t id, uint32_t length)
{
	switch (id) {
		case 3:
			return 255;
		case 4:
			return 200;
		case 9:
			return 32;
		case 13:
			return 12;
		case 0x80444556:
		case 0x80494E4F:
		case 0x8053594E:
			return 8;
		case 0x8053567E:
			return 4;
		default:
			return length;
	}
}

static int adouble_read(
		char *fname,
		ad_entry *e,
		uint16_t *ec)
{
	uint8_t b[12];

	FILE *fp = fopen(fname, "r");
	if (!fp) {
		printf("Unable to open file '%s' for reading\n", fname);
		return 1;
	}

	// magic number
	if ((fread(b, 1, 4, fp)) != 4) {
		printf("Error reading magic number in '%s'\n", fname);
		fclose(fp);
		return 1;
	}
	uint32_t magic = (b[0] << 24)
			+ (b[1] << 16)
			+ (b[2] << 8)
			+ b[3];
	if (magic != 0x00051607) {
		printf("Bad magic number (0x%X) in '%s'\n", magic, fname);
		fclose(fp);
		return 1;
	}

	// number of entries, just after preamble
	fseek(fp, 24, SEEK_SET);
	if ((fread(b, 1, 2, fp)) != 2) {
		printf("Error reading number of entries '%s'\n", fname);
		fclose(fp);
		return 1;
	}
	*ec = (b[0] << 8)
			+ b[1];
	if (*ec >= MAX_ENTRIES) {
		printf("More than max entries (%d) found in '%s'\n",
				*ec, fname);
		fclose(fp);
		return 1;
	}

	// get entry list
	for (uint16_t i = 0; i < *ec; i++) {
		// ID
		if ((fread(b, 1, 4, fp)) != 4) {
			printf("Error reading entry ID %d in '%s'\n",
					i, fname);
			fclose(fp);
			return 1;
		}
		e[i].id = (b[0] << 24)
				+ (b[1] << 16)
				+ (b[2] << 8)
				+ b[3];

		// offset
		if ((fread(b, 1, 4, fp)) != 4) {
			printf("Error reading entry offset %d in '%s'\n",
					i, fname);
			fclose(fp);
			return 1;
		}
		e[i].offset = (b[0] << 24)
				+ (b[1] << 16)
				+ (b[2] << 8)
				+ b[3];

		// length
		if ((fread(b, 1, 4, fp)) != 4) {
			printf("Error reading entry length %d in '%s'\n",
					i, fname);
			fclose(fp);
			return 1;
		}
		e[i].length = (b[0] << 24)
				+ (b[1] << 16)
				+ (b[2] << 8)
				+ b[3];
		if (e[i].length > MAX_ENTRY_SIZE) {
			printf("Max descriptor data exceeded, was %d in '%s'\n",
					e[i].length, fname);
			fclose(fp);
			return 1;
		}
	}

	// read in data for each entry
	for (uint16_t i = 0; i < *ec; i++) {
		fseek(fp, e[i].offset, SEEK_SET);

		e[i].data = malloc(e[i].length);
		if (e[i].data == NULL) {
			printf("Unable to allocate entry data memory\n");
			for (uint16_t j = 0; j < i; j++) {
				free(e[j].data);
			}
			fclose(fp);
			return 1;
		}

		if ((fread(e[i].data, 1, e[i].length, fp)) != e[i].length) {
			printf("Error reading data for EID %d in '%s'\n",
					e[i].id, fname);
			for (uint16_t j = 0; j <= i; j++) {
				free(e[j].data);
			}
			fclose(fp);
			return 1;
		}
	}

	fclose(fp);
	return 0;
}

static void adouble_sort(
		ad_entry *e,
		uint16_t ec,
		uint8_t headers)
{
	uint32_t sm, sj;
	uint16_t min;
	for (uint16_t i = 0; i < ec - 1; i++) {
		min = i;
		for (uint16_t j = i + 1; j < ec; j++) {
			if (headers) {
				sj = adouble_order_header(e[j].id);
				sm = adouble_order_header(e[min].id);
			} else {
				sj = adouble_order_data(e[j].id);
				sm = adouble_order_data(e[min].id);
			}

			if (sj < sm) {
				min = j;
			}
		}
		if (min != i) {
			ad_entry t = e[i];
			e[i] = e[min];
			e[min] = t;
		}
	}
}

static int adouble_write(
		char *fname,
		ad_entry *entries,
		uint16_t ec)
{
	// make a copy of entries to avoid scrambling original order
	ad_entry *e = malloc(sizeof(ad_entry) * ec);
	memcpy(e, entries, sizeof(ad_entry) * ec);

	// sort by data order for generating offsets
	adouble_sort(e, ec, 0);

	// calculate offsets
	uint32_t offset = 26 + ec * 12;
	for (uint8_t i = 0; i < ec; i++) {
		e[i].offset = offset;
		offset += adouble_size(e[i].id, e[i].length);
	}

	// re-sort by header order for writing that part
	adouble_sort(e, ec, 1);

	// format and write header
	uint8_t b[24] = {0};
	b[1] = 0x05;
	b[2] = 0x16;
	b[3] = 0x07;
	b[5] = 0x02;
	FILE *fp = fopen(fname, "w");
	if (!fp) {
		printf("Unable to open file '%s' for writing\n", fname);
		free(e);
		return 1;
	}

	if ((fwrite(b, 24, 1, fp)) != 1) {
		printf("Error writing header in '%s'\n", fname);
		free(e);
		fclose(fp);
		return 1;
	}
	b[0] = ec >> 8;
	b[1] = ec;
	if ((fwrite(b, 2, 1, fp)) != 1) {
		printf("Error writing header count in '%s'\n", fname);
		free(e);
		fclose(fp);
		return 1;
	}

	// write each entry
	for (uint8_t i = 0; i < ec; i++) {
		b[0] = e[i].id >> 24;
		b[1] = e[i].id >> 16;
		b[2] = e[i].id >> 8;
		b[3] = e[i].id;
		b[4] = e[i].offset >> 24;
		b[5] = e[i].offset >> 16;
		b[6] = e[i].offset >> 8;
		b[7] = e[i].offset;
		b[8] = e[i].length >> 24;
		b[9] = e[i].length >> 16;
		b[10] = e[i].length >> 8;
		b[11] = e[i].length;
		if ((fwrite(b, 12, 1, fp)) != 1) {
			printf("Error writing entry header %d in '%s'\n",
					i, fname);
			free(e);
			fclose(fp);
			return 1;
		}
	}

	// write data
	for (uint8_t i = 0; i < ec; i++) {
		if (e[i].length == 0) continue;
		if (fseek(fp, e[i].offset, SEEK_SET)) {
			printf("Error seeking while writing EID %d in '%s'\n",
					e[i].id, fname);
			free(e);
			fclose(fp);
			return 1;
		}
		if ((fwrite(e[i].data, e[i].length, 1, fp)) != 1) {
			printf("Error writing entry data for EID %d in '%s'\n",
					e[i].id, fname);
			free(e);
			fclose(fp);
			return 1;
		}
	}

	free(e);
	fclose(fp);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: admerge data_fork netatalk_fork");
		return 1;
	}

	// read files
	ad_entry df[MAX_ENTRIES], nf[MAX_ENTRIES];
	uint16_t nf_cnt, df_cnt;
	if (adouble_read(argv[1], df, &df_cnt)) {
		return EXIT_FAILURE;
	}
	if (adouble_read(argv[2], nf, &nf_cnt)) {
		return EXIT_FAILURE;
	}

	/*
	 * For each option in the data fork, find the existing one and
	 * replace it *or* insert a new one
	 */
	for (uint16_t dfi = 0; dfi < df_cnt; dfi++) {
		// find match if one exists
		uint16_t nfm = -1;
		for (uint nfi = 0; nfi < nf_cnt; nfi++) {
			if (df[dfi].id == nf[nfi].id) {
				nfm = nfi;
				break;
			}
		}

		if (nfm >= 0) {
			free(nf[nfm].data);
			nf[nfm].length = df[dfi].length;
			nf[nfm].data = df[dfi].data;
		} else {
			if (nf_cnt + 1 >= MAX_ENTRIES) {
				printf("Not enough space for appending data");
				return EXIT_FAILURE;
			}
			nf[nf_cnt++] = df[dfi];
		}
	}

	// overwrite
	if (adouble_write(argv[2], nf, nf_cnt)) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
