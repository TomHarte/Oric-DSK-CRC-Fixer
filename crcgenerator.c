//
//  crcgenerator.c
//  dskcrcfixer
//
//  Created by Thomas Harte on 02/01/2017.
//  Copyright © 2017 Thomas Harte. All rights reserved.
//

#include "crcgenerator.h"

#include <stdlib.h>

struct CRCGeneratorInternal {
	uint16_t xor_table[256];
	uint16_t value;
};

CRCGenerator *crc_create(const uint16_t polynomial)
{
	struct CRCGeneratorInternal *const generator = (struct CRCGeneratorInternal *)malloc(sizeof(struct CRCGeneratorInternal));

	generator->value = 0xffff;
	for(int c = 0; c < 256; c++)
	{
		uint16_t shift_value = (uint16_t)(c << 8);
		for(int b = 0; b < 8; b++)
		{
			const uint16_t exclusive_or = (shift_value&0x8000) ? polynomial : 0x0000;
			shift_value = (uint16_t)(shift_value << 1) ^ exclusive_or;
		}
		generator->xor_table[c] = shift_value;
	}

	return generator;
}

void crc_destroy(CRCGenerator *const generator)
{
	free(generator);
}

void crc_reset_to_value(CRCGenerator *const gen, const uint16_t value)
{
	struct CRCGeneratorInternal *const generator = (struct CRCGeneratorInternal *)gen;
	generator->value = value;
}

void crc_add_byte(CRCGenerator *const gen, const uint8_t byte)
{
	struct CRCGeneratorInternal *const generator = (struct CRCGeneratorInternal *)gen;
	generator->value = (uint16_t)((generator->value << 8) ^ generator->xor_table[(generator->value >> 8) ^ byte]);
}

uint16_t crc_get_value(const CRCGenerator *const gen)
{
	const struct CRCGeneratorInternal *const generator = (struct CRCGeneratorInternal *)gen;
	return generator->value;
}
