/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2006 Wolfgang Moser (http://d81.de)
 */

#include "gcr.h"

int gcr_decode(unsigned const char *gcr, unsigned char *decoded)
{
    unsigned char chkref[4], chksum = 0;
    int i,j;

    /* FIXME:
     * Let's introduce buffer length specifiers and sanity checks,
     * so the the following assumptions of length specifications
     * of 4 and 5 become true.
     */
    gcr_5_to_4_decode(gcr, chkref, 5, sizeof(chkref));
    gcr += 5;

    if(chkref[0] != 0x07)
    {
        return 4;
    }

        /* move over the remaining three bytes */
    for(j = 1; j < 4; j++, decoded++)
    {
        *decoded = chkref[j];
        chksum  ^= chkref[j];
    }

        /* main block processing loop */
    for(i = 1; i < BLOCKSIZE/4; i++)
    {
        gcr_5_to_4_decode(gcr, decoded, 5, 4);
        gcr += 5;

        for(j = 0; j < 4; j++, decoded++)
        {
            chksum ^= *decoded;
        }
    }

    gcr_5_to_4_decode(gcr, chkref, 5, 4);
        /* move over the remaining last byte */
    *decoded = chkref[0];
    chksum  ^= chkref[0];

    return (chkref[1] != chksum) ? 5 : 0;
}

int gcr_encode(unsigned const char *block, unsigned char *encoded)
{

    unsigned char chkref[4] = { 0x07, 0, 0, 0};
    int i,j;

    /* FIXME:
     * Let's introduce buffer length specifiers and sanity checks,
     * so the the following assumptions of length specifications
     * of 4 and 5 become true.
     */

        /* start with encoding the data block
         * identifier and the first three bytes
         */
    for(j = 1; j < 4; j++, block++)
    {
        chkref[j] = *block;
    }
       gcr_4_to_5_encode(chkref, encoded, sizeof(chkref), 5);
       encoded += 5;

        /* add the three bytes into one checksum */
    chkref[1] ^= (chkref[2] ^ chkref[3]);

        /* main block processing loop */
    for(i = 1; i < BLOCKSIZE/4; i++)
    {
        gcr_4_to_5_encode(block, encoded, 4, 5);
        encoded += 5;

        for(j = 0; j < 4; j++, block++)
        {
                /* directly encode the checksum */
            chkref[1] ^= *block;
        }
    }

        /* move over the remaining last byte */
    chkref[0]  = *block;
        /* add the last byte to the checksum */
    chkref[1] ^= *block;

    /* clear trailing unused bytes, not necessary but somehow nicer */
    chkref[2] = chkref[3] = 0;

    gcr_4_to_5_encode(chkref, encoded, 4, 5);

    return 0;
}
