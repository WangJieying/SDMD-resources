/*
 * File:   fileio.hpp
 * Author: yuri
 *
 * Created on July 20, 2011, 3:58 PM
 */

#ifndef FILEIO_HPP
#define	FILEIO_HPP
#include <stdint.h>

const uint8_t END_TAG = 0x38;
const uint8_t FORK_TAG = 0x39;
enum COLORSPACE {NONE, GRAY, RGB, HSV, YCC};

#define USE_CUDA 0
#define READUINT8(p)    (uint8_t ) *( (uint8_t  *) (p) ); ((p)++)
#define READUINT16(p)   (uint16_t) *( (uint16_t *) (p) ); ((p)+=2)
#define READUINT32(p)   (uint32_t) *( (uint32_t *) (p) ); ((p)+=4)
#define READINT8(p)     (int8_t  ) *( (int8_t   *) (p) ); ((p)++)
#define READINT16(p)    (int16_t ) *( (int16_t  *) (p) ); ((p)+=2)
#define READINT32(p)    (int32_t ) *( (int32_t  *) (p) ); ((p)+=4)
#endif	/* FILEIO_HPP */

