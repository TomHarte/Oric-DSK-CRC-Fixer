//
//  crcgenerator.h
//  dskcrcfixer
//
//  Created by Thomas Harte on 02/01/2017.
//  Copyright © 2017 Thomas Harte. All rights reserved.
//

#ifndef crcgenerator_h
#define crcgenerator_h

#include <stdint.h>

typedef void CRCGenerator;

CRCGenerator *crc_create(uint16_t polynomial);
void crc_destroy(CRCGenerator *generator);

void crc_reset_to_value(CRCGenerator *, uint16_t value);
void crc_add_byte(CRCGenerator *, uint8_t byte);
uint16_t crc_get_value(CRCGenerator *);

#endif /* crcgenerator_h */
