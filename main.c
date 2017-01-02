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

int main(int argc, const char * argv[]) {
	// Sanity check 1: did the user provide any arguments?
	if(argc < 2)
	{
		const char *app_name = argv[0] + strlen(argv[0]);
		while(app_name > argv[0] && app_name[0] != '/') app_name--;
		printf("Usage: %s [name of dsk]\n", &app_name[1]);
		return -1;
	}

	// Sanity check 2: does the first argument identify a file that can be opened for modification?
	FILE *dsk = fopen(argv[1], "r+");
	if(!dsk)
	{
		printf("Error: couldn't open %s for modification\n", argv[1]);
		return -2;
	}

	// Sanity check 3: does the file, now opened, contain the proper magic word?
	char magic_word[8];
	const char expected_word[] = "MFM_DISK";
	fread(magic_word, sizeof(magic_word[0]), sizeof(magic_word), dsk);
	if(memcmp(magic_word, expected_word, strlen(expected_word)))
	{
		fclose(dsk);
		printf("Error: %s doesn't look like an Oric MFM disk\n", argv[1]);
		return -2;
	}

	//
	// All sanity checks passed.
	//

	// Prep some metrics.
	int tracks_processed = 0;
	int crcs_fixed = 0;
	int crcs_found = 0;

	// Obtain a CRC generator.
	CRCGenerator *generator = crc_create(0x1021);

	// Seed the file position.
	long file_offset = 256;
	while(1)
	{
		// Attempt to read in the existing track; if we hit feof then that's the end of that, done.
		fseek(dsk, file_offset, SEEK_SET);
		uint8_t track_image[6250];
		fread(track_image, sizeof(track_image[0]), sizeof(track_image), dsk);
		if(feof(dsk)) break;

		tracks_processed++;
		file_offset += 6400;
	}

	printf("Completed; %d tracks found, %d CRCs checked, %d fixed\n", tracks_processed, crcs_found, crcs_fixed);

	crc_destroy(generator);
	fclose(dsk);
    return 0;
}
