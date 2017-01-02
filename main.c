//
//  main.c
//  dskcrcfixer
//
//  Created by Thomas Harte on 02/01/2017.
//  Copyright © 2017 Thomas Harte. All rights reserved.
//

#include <memory.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "crcgenerator.h"

static const size_t MFMDISK_track_length = 6250;
static const size_t MFMDISK_offset_of_first_track = 256;

static void seed_crc_generator(CRCGenerator *generator, uint32_t value)
{
	crc_reset_to_value(generator, 0xffff);
	crc_add_byte(generator, value >> 24);
	crc_add_byte(generator, (value >> 16)&0xff);
	crc_add_byte(generator, (value >> 8)&0xff);
	crc_add_byte(generator, value&0xff);
}

int main(int argc, const char * argv[]) {
	// Sanity check 1: did the user provide any arguments?
	if(argc < 2)
	{
		const char *app_name = argv[0] + strlen(argv[0]);
		while(app_name > argv[0] && app_name[0] != '/') app_name--;
		printf("Usage: %s [name of first dsk] [name of second disk] ...\n", &app_name[1]);
		return -1;
	}

	// Obtain a CRC generator.
	CRCGenerator *generator = crc_create(0x1021);

	// Act upon every file presented.
	for(int argument = 1; argument < argc; argument++)
	{
		// Sanity check 2: does the first argument identify a file that can be opened for modification?
		FILE *dsk = fopen(argv[argument], "r+");
		if(!dsk)
		{
			printf("%s: Error: couldn't open for modification\n", argv[argument]);
			continue;
		}

		// Sanity check 3: does the file, now opened, contain the proper magic word?
		char magic_word[8];
		const char expected_word[] = "MFM_DISK";
		fread(magic_word, sizeof(magic_word[0]), sizeof(magic_word), dsk);
		if(memcmp(magic_word, expected_word, strlen(expected_word)))
		{
			fclose(dsk);
			printf("%s: Error: doesn't look like an Oric MFM disk\n", argv[argument]);
			continue;
		}

		//
		// All sanity checks passed.
		//

		// Prep some metrics.
		int tracks_processed = 0;
		int crcs_fixed = 0;
		int crcs_found = 0;

		// Seed the file position.
		fseek(dsk, MFMDISK_offset_of_first_track, SEEK_SET);
		while(1)
		{
			// Attempt to read in the existing track; if we hit feof then that's the end of that, done.
			uint8_t track_image[MFMDISK_track_length];
			int crcs_fixed_prior_to_track = crcs_fixed;

			fread(track_image, sizeof(track_image[0]), sizeof(track_image), dsk);
			if(feof(dsk)) break;

			// Perform parsing to look for sectors.
			size_t track_pointer = 0;
			uint32_t byte_shift_register = 0;
			uint8_t most_recent_id_mark[4];
			while(track_pointer < MFMDISK_track_length)
			{
				byte_shift_register = (byte_shift_register << 8) | track_image[track_pointer];
				track_pointer++;

				if(byte_shift_register == 0xa1a1a1fe && track_pointer < MFMDISK_track_length - 6)
				{
					// Update metric.
					crcs_found++;

					// Parse an ID mark.
					memcpy(most_recent_id_mark, &track_image[track_pointer], sizeof(most_recent_id_mark));

					seed_crc_generator(generator, byte_shift_register);
					for(int c = 0; c < sizeof(most_recent_id_mark); c++) crc_add_byte(generator, most_recent_id_mark[c]);

					uint16_t found_crc = (track_image[track_pointer+4] << 8) | track_image[track_pointer+5];
					uint16_t intended_crc = crc_get_value(generator);
					if(intended_crc != found_crc)
					{
						// Update metric and CRC.
						crcs_fixed++;
						track_image[track_pointer+4] = (intended_crc >> 8);
						track_image[track_pointer+5] = intended_crc & 0xff;
					}

					track_pointer += 6;
				}
				else if(byte_shift_register == 0xa1a1a1fb && track_pointer < MFMDISK_track_length - (128 << most_recent_id_mark[3]))
				{
					// Update metric, seed CRC.
					crcs_found++;
					seed_crc_generator(generator, byte_shift_register);

					// Parse a sector, per the most recent ID mark.
					for(int c = 0; c < 128 << most_recent_id_mark[3]; c++)
					{
						crc_add_byte(generator, track_image[track_pointer]);
						track_pointer++;
					}

					uint16_t found_crc = (track_image[track_pointer] << 8) | track_image[track_pointer+1];
					uint16_t intended_crc = crc_get_value(generator);
					if(intended_crc != found_crc)
					{
						// Update metric and CRC.
						crcs_fixed++;
						track_image[track_pointer] = (intended_crc >> 8);
						track_image[track_pointer+1] = intended_crc & 0xff;
					}

					track_pointer += 2;
				}
			}

			// Write back if needed
			if(crcs_fixed_prior_to_track != crcs_fixed)
			{
				fseek(dsk, -MFMDISK_track_length, SEEK_CUR);
				fwrite(track_image, sizeof(track_image[0]), sizeof(track_image), dsk);
			}

			// Update metric and file position.
			tracks_processed++;
		}

		printf("%s: Completed; %d tracks found, %d CRCs checked, %d fixed\n", argv[argument], tracks_processed, crcs_found, crcs_fixed);

		fclose(dsk);
	}

	crc_destroy(generator);
    return 0;
}
