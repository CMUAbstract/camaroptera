#ifndef COMPRESS_H
#define COMPRESS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>

extern uint8_t frame[];

uint16_t rle();
uint16_t lzw();
uint16_t huffman();

/*************************************************
 * JPEG Encoder
 *************************************************/

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


/** Type of a JPEG encoder object */
typedef struct jpec_enc_t_ jpec_enc_t;

/*
 * Create a JPEG encoder with default quality factor
 * `img' specifies the pointer to aligned image data.
 * `w' specifies the image width in pixels.
 * `h' specifies the image height in pixels.
 *
 */
jpec_enc_t *jpec_enc_new(const uint8_t *img, uint16_t w, uint16_t h);
/*
 * `q` specifies the JPEG quality factor in 0..100
 */
jpec_enc_t *jpec_enc_new2(const uint8_t *img, uint16_t w, uint16_t h, int q);

/*
 * Run the JPEG encoding
 * `e` specifies the encoder object
 * `len` specifies the pointer to the variable into which the length of the
 * JPEG blob is assigned
 */
const uint8_t *jpec_enc_run(jpec_enc_t *e, int *len);

/*
 * Release a JPEG encoder object
 * `e` specifies the encoder object
 */
void jpec_enc_del(jpec_enc_t *e);

uint16_t do_compression(uint16_t n_bytes, uint8_t type, uint8_t quantize, uint8_t q_step, uint8_t jpec_q);

#endif
