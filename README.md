# Oric DSK CRC Fixer
Many Oric-targetted .DSK files have incorrect CRCs, owing to the tools that were used to construct them. Some therefore may not load on emulators that accurately implement disk controller CRC checking.

This tool scans a disk image for all standard-format ID and sector data marks, computes the correct CRCs and replaces those in the file if they are not already correct.

## Prequisites

A C99 compiler.

## Build Instructions

Compile the two .c files and link them together. E.g.

_gcc crcgenerator.c main.c -o crcdskfixer_

## Usage

_dskcrcfixer [file1] [file2] ..._
