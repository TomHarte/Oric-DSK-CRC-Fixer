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

/*!
	Opaquely represents a CRC generator. Use @c crc_create to construct and @c crc_destroy to dispose.
*/
typedef void CRCGenerator;

/*!
	Creates a new 16-bit CRC generator. It will be seeded with the value 0xffff.

	@param polynomial this CRC generator's polynomial.
	@returns a CRC generator with a heap footprint that you own.
	@sa @c crc_destroy to release resources associated with the CRC generator.
*/
CRCGenerator *crc_create(uint16_t polynomial);

/*!
	Releases all resources associated with @c generator.

	@param generator the generator to destroy.
*/
void crc_destroy(CRCGenerator *generator);

/*!
	Sets the current value for @c generator.

	@param generator the generator to set a value for.
	@param value the value to set.
*/
void crc_reset_to_value(CRCGenerator *generator, uint16_t value);

/*!
	Adds a byte to the CRC generator.

	@param generator the generator to add the byte to.
	@param byte the byte to add to the generator.
*/
void crc_add_byte(CRCGenerator *generator, uint8_t byte);

/*!
	@returns the current value of @c generator.
*/
uint16_t crc_get_value(const CRCGenerator *generator);

#endif /* crcgenerator_h */
