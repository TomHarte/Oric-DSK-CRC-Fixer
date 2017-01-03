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

/*
	Quick introduction to the Oric MFM_DISK file format:

	It opens with a 256-byte header, the first eight of which are the ASCII values for MFM_DISK.
	That's followed by a 32-bit little endian head count, then a 32-bit little endian track count,
	then a 32-bit little endian geometry type: either '1' to indicate that all the tracks for one side
	are listed contiguously, then all the tracks for the next, etc; or '2' to indicate that all the
	sides for one track are listed contiguously, then all the sides for the next track, etc.

	For this tool's purposes, merely confirming that the MFM_DISK signature matches is sufficient. It'll
	then just run for as many tracks as there are, in whatever order they come.

	After the 256-byte header, each track occupies 6400 bytes. The first 6250 of those are defined to be
	the track contents.

	The file format does not include clock bits and guarantees that things intended logically to form
	bytes on the disk surface will be byte-aligned in the file.

	It is not explicit in disambiguating MFM sync words from properly encoded variants but the designer
	advocates that a controller-esque parsing be used, treating those A1 and C2 bytes not within sector
	bodies as the MFM sync words. This code assumes the converse logic to be implicit: those A1 and C2
	bytes that are within sector bodies are not MFM sync words.
*/

static const size_t MFMDISK_track_length = 6250;
static const size_t MFMDISK_distance_between_tracks = 150;
static const size_t MFMDISK_offset_of_first_track = 256;
static const char MFMDISK_expected_signature[] = "MFM_DISK";

static const uint16_t CRC_polynomial = 0x1021;
static const uint16_t CRC_reload_value = 0xffff;

/*!
	Rsets the CRC generator to @c CRC_reload_value and feeds it the four bytes in @c value
	in descending order of significance — most significant byte first, then second-most,
	etc.
	
	@param generator the CRC generator to seed.
	@param value the four bytes to feed the CRC generator after resetting it to @c CRC_reload_value.
*/
static void seed_crc_generator(CRCGenerator *generator, uint32_t value)
{
	crc_reset_to_value(generator, CRC_reload_value);
	crc_add_byte(generator, value >> 24);
	crc_add_byte(generator, (value >> 16)&0xff);
	crc_add_byte(generator, (value >> 8)&0xff);
	crc_add_byte(generator, value&0xff);
}

int main(int argc, const char *argv[])
{
	// Sanity check 1: did the user provide any arguments?
	if(argc < 2)
	{
		const char *app_name = argv[0] + strlen(argv[0]);
		while(app_name > argv[0] && app_name[0] != '/') app_name--;
		printf("Usage: %s [name of first dsk] [name of second disk] ...\n", &app_name[1]);
		return -1;
	}

	// Obtain a CRC generator.
	CRCGenerator *const generator = crc_create(CRC_polynomial);

	// Act upon every file presented.
	for(int argument = 1; argument < argc; argument++)
	{
		// Sanity check 2: does the first argument identify a file that can be opened for modification?
		FILE *const dsk = fopen(argv[argument], "r+");
		if(!dsk)
		{
			printf("%s: Error; couldn't open for modification\n", argv[argument]);
			continue;
		}

		// Sanity check 3: does the file, now opened, contain the proper magic word?
		char signature[8];
		size_t length = fread(signature, sizeof(signature[0]), sizeof(signature), dsk);
		if(length != strlen(MFMDISK_expected_signature) || memcmp(signature, MFMDISK_expected_signature, strlen(MFMDISK_expected_signature)))
		{
			fclose(dsk);
			printf("%s: Error; doesn't look like an Oric MFM disk\n", argv[argument]);
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
			const int crcs_fixed_prior_to_track = crcs_fixed;

			const size_t bytes_read = fread(track_image, sizeof(track_image[0]), sizeof(track_image), dsk);
			if(bytes_read != sizeof(track_image)) break;

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

					const uint16_t found_crc = (track_image[track_pointer+4] << 8) | track_image[track_pointer+5];
					const uint16_t intended_crc = crc_get_value(generator);
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

					const uint16_t found_crc = (track_image[track_pointer] << 8) | track_image[track_pointer+1];
					const uint16_t intended_crc = crc_get_value(generator);
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

			// Write back if needed.
			if(crcs_fixed_prior_to_track != crcs_fixed)
			{
				fseek(dsk, -MFMDISK_track_length, SEEK_CUR);
				fwrite(track_image, sizeof(track_image[0]), sizeof(track_image), dsk);
			}

			// Update metric and file position.
			fseek(dsk, MFMDISK_distance_between_tracks, SEEK_CUR);
			tracks_processed++;
		}

		printf("%s: Completed; %d tracks found, %d CRCs checked, %d fixed\n", argv[argument], tracks_processed, crcs_found, crcs_fixed);

		fclose(dsk);
	}

	crc_destroy(generator);
	return 0;
}
