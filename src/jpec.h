/**
 * Copyright (c) 2012-2016 Moodstocks SAS
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
#ifndef JPEC_H
#define JPEC_H

/* Common headers */
#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/** Extensible byte buffer */
typedef struct jpec_buffer_t_ {
  uint8_t *stream;                      /* byte buffer */
  int len;                              /* current length */
  int siz;                              /* maximum size */
} jpec_buffer_t;

jpec_buffer_t *jpec_buffer_new(void);
jpec_buffer_t *jpec_buffer_new2(int siz);

void jpec_buffer_del(jpec_buffer_t *b);
void jpec_buffer_write_byte(jpec_buffer_t *b, int val);
void jpec_buffer_write_2bytes(jpec_buffer_t *b, int val);

/** Structure used to hold and process an image 8x8 block */
typedef struct jpec_block_t_ {
  float dct[64];            /* DCT coefficients */
  int quant[64];            /* Quantization coefficients */
  int zz[64];               /* Zig-Zag coefficients */
  int len;                  /* Length of Zig-Zag coefficients */
} jpec_block_t;

/** Skeleton for an Huffman entropy coder */
typedef struct jpec_huff_skel_t_ {
  void *opq;
  void (*del)(void *opq);
  void (*encode_block)(void *opq, jpec_block_t *block, jpec_buffer_t *buf);
} jpec_huff_skel_t;

/** JPEG encoder */
struct jpec_enc_t_ {
  /** Input image data */
  const uint8_t *img;                   /* image buffer */
  uint16_t w;                           /* image width */
  uint16_t h;                           /* image height */
  uint16_t w8;                          /* w rounded to upper multiple of 8 */
  /** JPEG extensible byte buffer */
  jpec_buffer_t *buf;
  /** Compression parameters */
  int qual;                             /* JPEG quality factor */
  int dqt[64];                          /* scaled quantization matrix */
  /** Current 8x8 block */
  int bmax;                             /* maximum number of blocks (N) */
  int bnum;                             /* block number in 0..N-1 */
  uint16_t bx;                          /* block start X */
  uint16_t by;                          /* block start Y */
  jpec_block_t block;                   /* block data */
  /** Huffman entropy coder */
  jpec_huff_skel_t *hskel;
};

/** Entropy coding data that hold state along blocks */
typedef struct jpec_huff_state_t_ {
  int32_t buffer;             /* bits buffer */
  int nbits;                  /* number of bits remaining in buffer */
  int dc;                     /* DC coefficient from previous block (or 0) */
  jpec_buffer_t *buf;         /* JPEG global buffer */
} jpec_huff_state_t;

/** Type of an Huffman JPEG encoder */
typedef struct jpec_huff_t_ {
  jpec_huff_state_t state;    /* state from previous block encoding */
} jpec_huff_t;

/** Skeleton initialization */
void jpec_huff_skel_init(jpec_huff_skel_t *skel);

jpec_huff_t *jpec_huff_new(void);

void jpec_huff_del(jpec_huff_t *h);

void jpec_huff_encode_block(jpec_huff_t *h, jpec_block_t *block, jpec_buffer_t *buf);


/*************************************************
 * JPEG Encoder
 *************************************************/
 
/* -------------------------------------------------
 * LIMITATIONS
 * -------------------------------------------------
 * - Grayscale *only* (monochrome JPEG file): no support for color
 * - Baseline DCT-based  (SOF0), JFIF 1.01 (APP0) JPEG
 * - Block size of 8x8 pixels *only*
 * - Default quantization and Huffman tables *only*
 * - No border filling support: the input image *MUST* represent an integer
 *   number of blocks, i.e. each dimension must be a multiple of 8
 */

/** Type of a JPEG encoder object */
typedef struct jpec_enc_t_ jpec_enc_t;

/*
 * Create a JPEG encoder with default quality factor
 * `img' specifies the pointer to aligned image data.
 * `w' specifies the image width in pixels.
 * `h' specifies the image height in pixels.
 * Because the returned encoder is allocated by this function, it should be
 * released with the `jpec_enc_del' call when it is no longer useful.
 * Note: for efficiency the image data is *NOT* copied and the encoder just
 * retains a pointer to it. Thus the image data must not be deleted
 * nor change until the encoder object gets deleted.
 */
jpec_enc_t *jpec_enc_new(const uint8_t *img, uint16_t w, uint16_t h);
/*
 * `q` specifies the JPEG quality factor in 0..100
 */
jpec_enc_t *jpec_enc_new2(const uint8_t *img, uint16_t w, uint16_t h, int q);

/*
 * Release a JPEG encoder object
 * `e` specifies the encoder object
 */
void jpec_enc_del(jpec_enc_t *e);

/*
 * Run the JPEG encoding
 * `e` specifies the encoder object
 * `len` specifies the pointer to the variable into which the length of the
 * JPEG blob is assigned
 * If successful, the return value is the pointer to the JPEG blob. `NULL` is
 * returned if an error occurred.
 * Note: the caller should take care to copy or save the JPEG blob before
 * calling `jpec_enc_del` since the blob will no longer be maintained after.
 */
const uint8_t *jpec_enc_run(jpec_enc_t *e, uint16_t *len);

#endif
