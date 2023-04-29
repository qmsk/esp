#define UNPACK_UINT32_L(data, step, index, shift) ( \
    (((data[0 * (step) + (index)] >> (shift)) & 0xff) << 24) \
  | (((data[1 * (step) + (index)] >> (shift)) & 0xff) << 16) \
  | (((data[2 * (step) + (index)] >> (shift)) & 0xff) << 8) \
  | (((data[3 * (step) + (index)] >> (shift)) & 0xff) << 0) \
)
#define UNPACK_UINT32_H(data, step, index, shift) ( \
    (((data[4 * (step) + (index)] >> (shift)) & 0xff) << 24) \
  | (((data[5 * (step) + (index)] >> (shift)) & 0xff) << 16) \
  | (((data[6 * (step) + (index)] >> (shift)) & 0xff) << 8) \
  | (((data[7 * (step) + (index)] >> (shift)) & 0xff) << 0) \
)

static inline void i2s_out_transpose_uint32(uint32_t *x, uint32_t *y)
{
  uint32_t t;

  t = (*x ^ (*x >> 7)) & 0x00AA00AA; *x = *x ^ t ^ (t << 7);
  t = (*y ^ (*y >> 7)) & 0x00AA00AA; *y = *y ^ t ^ (t << 7);

  t = (*x ^ (*x >> 14)) & 0x0000CCCC; *x = *x ^ t ^ (t << 14);
  t = (*y ^ (*y >> 14)) & 0x0000CCCC; *y = *y ^ t ^ (t << 14);

  t = (*x & 0xF0F0F0F0) | ((*y >> 4) & 0x0F0F0F0F);
  *y = ((*x << 4) & 0xF0F0F0F0) | (*y & 0x0F0F0F0F);
  *x = t;

  // swap the 8-bit bytes of each 16-bit half of the 32-bit sample for the I2S 8-bit parallel fifo mode
  // TODO: is it possible to merge this into the above bit-wrangling?
  *x = ((*x << 8) & 0xFF00FF00) | ((*x >> 8) & 0x00FF00FF);
  *y = ((*y << 8) & 0xFF00FF00) | ((*y >> 8) & 0x00FF00FF);
}

static inline void i2s_out_transpose_serial32(uint32_t data, uint32_t buf[1])
{
  buf[0] = __builtin_bswap32(data);
}

// transpose an array of data[8][step] parallel 8x8-bit values at [0..8][index] -> 2x32-bit I2S 8-bit FIFO values at *buf
static inline void i2s_out_transpose_parallel8x8(const uint8_t data[8], unsigned step, unsigned index, uint32_t buf[2])
{
  buf[0] = UNPACK_UINT32_L(data, step, index, 0);
  buf[1] = UNPACK_UINT32_H(data, step, index, 0);

  i2s_out_transpose_uint32(&buf[0], &buf[1]);
}

// transpose an array of data[8][step] parallel 8x16-bit values at [0..8][index] -> 4x32-bit I2S 8-bit FIFO values at *buf
static inline void i2s_out_transpose_parallel8x16(uint16_t data[], unsigned step, unsigned index, uint32_t buf[4])
{
  buf[0] = UNPACK_UINT32_L(data, step, index, 8);
  buf[1] = UNPACK_UINT32_H(data, step, index, 8);
  buf[2] = UNPACK_UINT32_L(data, step, index, 0);
  buf[3] = UNPACK_UINT32_H(data, step, index, 0);

  i2s_out_transpose_uint32(&buf[0], &buf[1]);
  i2s_out_transpose_uint32(&buf[2], &buf[3]);
}

// transpose an array of data[8][step] parallel 8x32-bit values at [0..8][index] -> 8x32-bit I2S 8-bit FIFO values at *buf
static inline void i2s_out_transpose_parallel8x32(uint32_t data[], unsigned step, unsigned index, uint32_t buf[8])
{
  buf[0] = UNPACK_UINT32_L(data, step, index, 0);
  buf[1] = UNPACK_UINT32_H(data, step, index, 0);
  buf[2] = UNPACK_UINT32_L(data, step, index, 8);
  buf[3] = UNPACK_UINT32_H(data, step, index, 8);
  buf[4] = UNPACK_UINT32_L(data, step, index, 16);
  buf[5] = UNPACK_UINT32_H(data, step, index, 16);
  buf[6] = UNPACK_UINT32_L(data, step, index, 24);
  buf[7] = UNPACK_UINT32_H(data, step, index, 24);

  i2s_out_transpose_uint32(&buf[0], &buf[1]);
  i2s_out_transpose_uint32(&buf[2], &buf[3]);
  i2s_out_transpose_uint32(&buf[4], &buf[5]);
  i2s_out_transpose_uint32(&buf[6], &buf[7]);
}
