#include "fseq.h"
#include "file.h"
#include <logging.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char fseq_v2_id[] = FSEQ_V2_ID;

static int fseq_read_header_v2(struct fseq_header_v2 *header, FILE *file)
{
  if (!fread(header, sizeof(*header), 1, file)) {
    LOG_ERROR("fread size=%u: %s", sizeof(*header), strerror(errno));
    return -1;
  }

  if (memcmp(header->id, fseq_v2_id, sizeof(header->id))) {
    LOG_WARN("invalid ID=%02x%02x%02x%02x",
      header->id[0],
      header->id[1],
      header->id[2],
      header->id[3]
    );
    return -1;
  }

  if (header->major_version != FSEQ_V2_MAJOR_VERSION) {
    LOG_WARN("invalid major version = %u.%u", header->major_version, header->minor_version);
    return -1;
  } else if (header->minor_version > FSEQ_V2_MINOR_VERSION) {
    LOG_WARN("invalid minor version = %u.%u", header->major_version, header->minor_version);
    return -1;
  }

  LOG_INFO("id=%c%c%c%c minor_version=%u major_version=%u header_length=%u flags=%#02x",
    header->id[0], header->id[1], header->id[2], header->id[3],
    header->minor_version,
    header->major_version,
    header->header_length,
    header->flags
  );
  LOG_INFO("data_offset=%u channel_count=%u frame_count=%u frame_step_ms=%u",
    header->data_offset,
    header->channel_count,
    header->frame_count,
    header->frame_step_ms
  );
  LOG_INFO("compression_type=%u compression_block_count=%u",
    header->compression_type,
    header->compression_block_count
  );
  LOG_INFO("sparse_range_count=%u",
    header->sparse_range_count
  );

  return 0;
}

static int fseq_read_header(struct fseq *fseq)
{
  int err;

  if ((err = fseq_read_header_v2(&fseq->header, fseq->file))) {
    LOG_ERROR("fseq_read_header_v2");
    return err;
  }

  return 0;
}

static int fseq_read_compression_blocks(struct fseq *fseq)
{
  unsigned count = fseq->header.compression_block_count;

  if (!(fseq->compression_blocks = calloc(count, sizeof(*fseq->compression_blocks))) && count) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (fread(fseq->compression_blocks, sizeof(*fseq->compression_blocks), count, fseq->file) < count) {
    LOG_ERROR("fread size=%u count=%u: %s", sizeof(*fseq->compression_blocks), count, strerror(errno));
    return -1;
  }

  for (unsigned i = 0; i < count; i++) {
    LOG_INFO("compression_blocks[%d]: frame_index=%u length=%u", i,
      fseq->compression_blocks[i].frame_index,
      fseq->compression_blocks[i].length
    );
  }

  return 0;
}

static int fseq_read_sparse_ranges(struct fseq *fseq)
{
  unsigned count = fseq->header.sparse_range_count;

  if (!(fseq->sparse_ranges = calloc(count, sizeof(*fseq->sparse_ranges))) && count) {
    LOG_ERROR("calloc");
    return -1;
  }

  // XXX: 6-byte unaligned struct?
  if (fread(fseq->sparse_ranges, sizeof(*fseq->sparse_ranges), count, fseq->file) < count) {
    LOG_ERROR("fread size=%u count=%u: %s", sizeof(*fseq->sparse_ranges), count, strerror(errno));
    return -1;
  }

  for (unsigned i = 0; i < count; i++) {
    LOG_INFO("sparse_ranges[%d]: start_index=%u end_offset=%u", i,
      fseq->sparse_ranges[i].start_index,
      fseq->sparse_ranges[i].end_offset
    );
  }

  return 0;
}

static struct fseq_variable_header *new_fseq_variable_header(struct fseq *fseq, size_t size)
{
  struct fseq_variable_header **headers;
  struct fseq_variable_header *header;

  if (!(headers = realloc(fseq->variable_headers, fseq->variable_headers_count + 1))) {
    LOG_ERROR("realloc count=%u", fseq->variable_headers_count + 1);
    return NULL;
  } else {
    fseq->variable_headers = headers;
  }

  if (!(header = malloc(size))) {
    LOG_ERROR("malloc size=%u", size);
    return NULL;
  } else {
    fseq->variable_headers[fseq->variable_headers_count] = header;
    fseq->variable_headers_count++;
  }

  return header;
}

static int fseq_read_variable_headers(struct fseq *fseq)
{
  while (ftell(fseq->file) + 4 <= fseq->header.data_offset) {
    struct fseq_variable_header header, *h;

    if (!fread(&header, sizeof(header), 1, fseq->file)) {
      LOG_ERROR("fread size=%u: %s", sizeof(header), strerror(errno));
      return -1;
    }

    if (header.length < sizeof(header)) {
      LOG_WARN("invalid length=%u", header.length);
      return -1;
    }

    if (!(h = new_fseq_variable_header(fseq, header.length))) {
      LOG_ERROR("new_fseq_variable_header");
      return -1;
    }

    memcpy(h, &header, sizeof(header));

    if (!fread(h->data, h->length - sizeof(header), 1, fseq->file)) {
      LOG_ERROR("fread length=%u: %s", h->length - sizeof(header), strerror(errno));
      return -1;
    }

    LOG_INFO("variable_headers[]: length=%u code=%#06x", h->length, h->code);
  }

  return 0;
}

int fseq_read_headers(struct fseq *fseq)
{
  int err;

  if ((err = fseq_read_header(fseq))) {
    LOG_ERROR("fseq_read_header");
    return err;
  }

  if ((err = fseq_read_compression_blocks(fseq))) {
    LOG_ERROR("fseq_read_compression_blocks");
    return -1;
  }

  if ((err = fseq_read_sparse_ranges(fseq))) {
    LOG_ERROR("fseq_read_sparse_ranges");
    return -1;
  }

  if ((err = fseq_read_variable_headers(fseq))) {
    LOG_ERROR("fseq_read_variable_headers");
    return err;
  }

  return 0;
}

int fseq_seek_frame(struct fseq *fseq, unsigned frame)
{
  unsigned offset = fseq->header.data_offset + frame * fseq->header.channel_count;

  if (fseek(fseq->file, offset, SEEK_SET)) {
    LOG_ERROR("fseek %u: %s", offset, strerror(errno));
    return -1;
  }

  return 0;
}

int fseq_read_frame(struct fseq *fseq, struct fseq_frame *frame)
{
  if (!fread(frame->buf, frame->size, 1, fseq->file)) {
    LOG_ERROR("fread %ux1: %s", frame->size, strerror(errno));
    return -1;
  }

  return 0;
}
