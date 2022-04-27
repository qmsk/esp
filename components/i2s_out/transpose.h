// code adapted from https://github.com/hcs0/Hackers-Delight/blob/master/transpose8.c.txt
static inline void i2s_out_transpose_parallel8x8(uint8_t data[8], uint32_t buf[2])
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

// split a 2x8x8 of two sets of 8 parallel 8-bit values -> 2x2x32 two sets of two 32-bit LSB values
static inline void i2s_out_transpose_parallel8x16(uint16_t data[8], uint32_t buf[4])
{
  uint32_t a, b, c, d, t;

  a = ((data[0] >> 8) << 24) | ((data[1] >> 8) << 16) | ((data[2] >> 8) << 8) | (data[3] >> 8);
  b = ((data[4] >> 8) << 24) | ((data[5] >> 8) << 16) | ((data[6] >> 8) << 8) | (data[7] >> 8);

  c = ((data[0] & 0xff) << 24) | ((data[1] & 0xff) << 16) | ((data[2] & 0xff) << 8) | (data[3] & 0xff);
  d = ((data[4] & 0xff) << 24) | ((data[5] & 0xff) << 16) | ((data[6] & 0xff) << 8) | (data[7] & 0xff);

  t = (a ^ (a >> 7)) & 0x00AA00AA; a = a ^ t ^ (t << 7);
  t = (b ^ (b >> 7)) & 0x00AA00AA; b = b ^ t ^ (t << 7);
  t = (c ^ (c >> 7)) & 0x00AA00AA; c = c ^ t ^ (t << 7);
  t = (d ^ (d >> 7)) & 0x00AA00AA; d = d ^ t ^ (t << 7);

  t = (a ^ (a >> 14)) & 0x0000CCCC; a = a ^ t ^ (t << 14);
  t = (b ^ (b >> 14)) & 0x0000CCCC; b = b ^ t ^ (t << 14);
  t = (c ^ (c >> 14)) & 0x0000CCCC; c = c ^ t ^ (t << 14);
  t = (d ^ (d >> 14)) & 0x0000CCCC; d = d ^ t ^ (t << 14);

  buf[0] = (a & 0xF0F0F0F0) | ((b >> 4) & 0x0F0F0F0F);
  buf[1] = ((a << 4) & 0xF0F0F0F0) | (b & 0x0F0F0F0F);

  buf[2] = (c & 0xF0F0F0F0) | ((d >> 4) & 0x0F0F0F0F);
  buf[3] = ((c << 4) & 0xF0F0F0F0) | (d & 0x0F0F0F0F);

  // swap the 8-bit bytes of each 16-bit half of the 32-bit sample for the I2S 8-bit parallel fifo mode
  // TODO: is it possible to merge this into the above bit-wrangling?
  buf[0] = ((buf[0] << 8) & 0xFF00FF00) | ((buf[0] >> 8) & 0x00FF00FF);
  buf[1] = ((buf[1] << 8) & 0xFF00FF00) | ((buf[1] >> 8) & 0x00FF00FF);
  buf[2] = ((buf[2] << 8) & 0xFF00FF00) | ((buf[2] >> 8) & 0x00FF00FF);
  buf[3] = ((buf[3] << 8) & 0xFF00FF00) | ((buf[3] >> 8) & 0x00FF00FF);
}
