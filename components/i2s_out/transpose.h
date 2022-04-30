// code adapted from https://github.com/hcs0/Hackers-Delight/blob/master/transpose8.c.txt
static inline void i2s_out_transpose_parallel8x8(const uint8_t data[8], uint32_t buf[2])
{
  uint32_t x, y, t;

  x = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
  y = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | (data[7]);

  t = (x ^ (x >> 7)) & 0x00AA00AA; x = x ^ t ^ (t << 7);
  t = (y ^ (y >> 7)) & 0x00AA00AA; y = y ^ t ^ (t << 7);

  t = (x ^ (x >> 14)) & 0x0000CCCC; x = x ^ t ^ (t << 14);
  t = (y ^ (y >> 14)) & 0x0000CCCC; y = y ^ t ^ (t << 14);

  buf[0] = (x & 0xF0F0F0F0) | ((y >> 4) & 0x0F0F0F0F);
  buf[1] = ((x << 4) & 0xF0F0F0F0) | (y & 0x0F0F0F0F);

  // swap the 8-bit bytes of each 16-bit half of each 32-bit sample to account for the I2S 8-bit parallel mode fifo quirks
  // TODO: is it possible to merge this into the above bit-wrangling?
  buf[0] = ((buf[0] << 8) & 0xFF00FF00) | ((buf[0] >> 8) & 0x00FF00FF);
  buf[1] = ((buf[1] << 8) & 0xFF00FF00) | ((buf[1] >> 8) & 0x00FF00FF);
}

// transpose an array of data[8][step] parallel 8x16-bit values at [0..8][index] -> 4x32-bit I2S 8-bit FIFO values at *buf
static inline void i2s_out_transpose_parallel8x16(uint16_t data[], unsigned step, unsigned index, uint32_t buf[4])
{
  uint32_t a, b, c, d, t;

  a = ((data[0 * step + index] >> 8) << 24) | ((data[1 * step + index] >> 8) << 16) | ((data[2 * step + index] >> 8) << 8) | (data[3 * step + index] >> 8);
  b = ((data[4 * step + index] >> 8) << 24) | ((data[5 * step + index] >> 8) << 16) | ((data[6 * step + index] >> 8) << 8) | (data[7 * step + index] >> 8);

  c = ((data[0 * step + index] & 0xff) << 24) | ((data[1 * step + index] & 0xff) << 16) | ((data[2 * step + index] & 0xff) << 8) | (data[3 * step + index] & 0xff);
  d = ((data[4 * step + index] & 0xff) << 24) | ((data[5 * step + index] & 0xff) << 16) | ((data[6 * step + index] & 0xff) << 8) | (data[7 * step + index] & 0xff);

  t = (a ^ (a >> 7)) & 0x00AA00AA; a = a ^ t ^ (t << 7);
  t = (b ^ (b >> 7)) & 0x00AA00AA; b = b ^ t ^ (t << 7);
  t = (c ^ (c >> 7)) & 0x00AA00AA; c = c ^ t ^ (t << 7);
  t = (d ^ (d >> 7)) & 0x00AA00AA; d = d ^ t ^ (t << 7);

  t = (a ^ (a >> 14)) & 0x0000CCCC; a = a ^ t ^ (t << 14);
  t = (b ^ (b >> 14)) & 0x0000CCCC; b = b ^ t ^ (t << 14);
  t = (c ^ (c >> 14)) & 0x0000CCCC; c = c ^ t ^ (t << 14);
  t = (d ^ (d >> 14)) & 0x0000CCCC; d = d ^ t ^ (t << 14);

  t = (a & 0xF0F0F0F0) | ((b >> 4) & 0x0F0F0F0F);
  b = ((a << 4) & 0xF0F0F0F0) | (b & 0x0F0F0F0F);
  a = t;

  t = (c & 0xF0F0F0F0) | ((d >> 4) & 0x0F0F0F0F);
  d = ((c << 4) & 0xF0F0F0F0) | (d & 0x0F0F0F0F);
  c = t;

  // swap the 8-bit bytes of each 16-bit half of the 32-bit sample for the I2S 8-bit parallel fifo mode
  // TODO: is it possible to merge this into the above bit-wrangling?
  buf[0] = ((a << 8) & 0xFF00FF00) | ((a >> 8) & 0x00FF00FF);
  buf[1] = ((b << 8) & 0xFF00FF00) | ((b >> 8) & 0x00FF00FF);
  buf[2] = ((c << 8) & 0xFF00FF00) | ((c >> 8) & 0x00FF00FF);
  buf[3] = ((d << 8) & 0xFF00FF00) | ((d >> 8) & 0x00FF00FF);
}
