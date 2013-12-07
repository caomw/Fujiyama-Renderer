/*
Copyright (c) 2011-2013 Hiroshi Tsubokawa
See LICENSE and README
*/

#include "fj_iff.h"
#include "fj_memory.h"
#include <string.h>

struct IffFile {
  FILE *file;
};

IffFile *IffOpen(const char *filename, const char *modes)
{
  IffFile *iff = FJ_MEM_ALLOC(IffFile);
  if (iff == NULL) {
    return NULL;
  }

  iff->file = fopen(filename, modes);
  if (iff->file == NULL) {
    IffClose(iff);
    return NULL;
  }

  return iff;
}

void IffClose(IffFile *iff)
{
  if (iff == NULL) {
    return;
  }

  fclose(iff->file);
  FJ_MEM_FREE(iff);
}

static size_t iff_read(const IffFile *iff, void *data, size_t size, size_t count)
{
  return fread(data, size, count, iff->file);
}

static size_t iff_write(IffFile *iff, const void *data, size_t size, size_t count)
{
  return fwrite(data, size, count, iff->file);
}

static long iff_tell(const IffFile *iff)
{
  return ftell(iff->file);
}

enum {
  IFF_SEEK_SET = SEEK_SET,
  IFF_SEEK_CUR = SEEK_CUR,
  IFF_SEEK_END = SEEK_END
};
static int iff_seek(IffFile *iff, long offset, int base)
{
  return fseek(iff->file, offset, base);
}

#define DEFINE_READ_WRITE_(SUFFIX,TYPE) \
size_t IffWrite##SUFFIX(IffFile *iff, const TYPE *data, size_t count) \
{ \
  const size_t nwrotes = iff_write(iff, data, sizeof(*data), count); \
  return nwrotes * sizeof(*data); \
} \
size_t IffRead##SUFFIX(IffFile *iff, TYPE *data, size_t count) \
{ \
  const size_t nreads = fread(data, sizeof(*data), count, iff->file); \
  return nreads * sizeof(*data); \
}
DEFINE_READ_WRITE_(Int8, int8_t)
DEFINE_READ_WRITE_(Int16, int16_t)
DEFINE_READ_WRITE_(Int32, int32_t)
DEFINE_READ_WRITE_(Int64, int64_t)

size_t IffReadString(IffFile *iff, char *data)
{
  size_t data_size = 0;
  const size_t start = iff_tell(iff);
  iff_read(iff, &data_size, sizeof(data_size), 1);
  iff_read(iff, data,       sizeof(*data),     data_size);
  return iff_tell(iff) - start;
}

size_t IffWriteString(IffFile *iff, const char *data)
{
  const size_t data_size = sizeof(*data) * (strlen(data) + 1);
  const size_t start = iff_tell(iff);
  iff_write(iff, &data_size, sizeof(data_size), 1);
  iff_write(iff, data,       data_size,         1);
  return iff_tell(iff) - start;
}

static void write_chunk_id(IffFile *iff, const char *chunk_id)
{
  char id[CHUNK_ID_SIZE] = {'\0'};
  strncpy(id, chunk_id, CHUNK_ID_SIZE);
  iff_write(iff, id, CHUNK_ID_SIZE, 1);
}

#define DEFINE_WRITE_CHUNK_(SUFFIX,TYPE) \
size_t IffWriteChunk##SUFFIX(IffFile *iff, const char *chunk_id, \
    const TYPE *data, size_t count) \
{ \
  const size_t data_size = count * sizeof(*data); \
  const size_t start = iff_tell(iff); \
  write_chunk_id(iff, chunk_id); \
  iff_write(iff, &data_size, sizeof(data_size), 1); \
  iff_write(iff, data,       data_size,         1); \
  return iff_tell(iff) - start; \
}

DEFINE_WRITE_CHUNK_(Int8, int8_t)
DEFINE_WRITE_CHUNK_(Int16, int16_t)
DEFINE_WRITE_CHUNK_(Int32, int32_t)
DEFINE_WRITE_CHUNK_(Int64, int64_t)

void IffWriteChunkGroupBegin(IffFile *iff, const char *chunk_id, DataSize *begin_pos)
{
  const DataSize dummy_data_size = 3;
  write_chunk_id(iff, chunk_id);
  iff_write(iff, &dummy_data_size, sizeof(dummy_data_size), 1);
  *begin_pos = iff_tell(iff);
}

void IffWriteChunkGroupEnd(IffFile *iff, size_t begin_pos)
{
  long bytes = iff_tell(iff) - begin_pos;

  if (bytes % 2 == 1) {
    int8_t c = '\0';
    iff_write(iff, &c, sizeof(c), 1);
    bytes++;
  }

  iff_seek(iff, -bytes - sizeof(DataSize), IFF_SEEK_CUR);
  iff_write(iff, &bytes, sizeof(bytes), 1);
  iff_seek(iff, bytes, IFF_SEEK_CUR);
}

void IffReadChunkGroupBegin(IffFile *iff, IffChunk *group_chunk)
{
}

void IffReadChunkGroupEnd(IffFile *iff, IffChunk *group_chunk)
{
  IffSkipCurrentChunk(iff, group_chunk);
}

int IffReadNextChunk(IffFile *iff, IffChunk *chunk)
{
  strncpy(chunk->id, "", CHUNK_ID_SIZE);

  iff_read(iff, chunk->id, sizeof(chunk->id), 1);
  if (IffChunkMatch(chunk, "")) {
    return 0;
  }

  iff_read(iff, &chunk->data_size, sizeof(chunk->data_size), 1);
  chunk->data_head = iff_tell(iff);
  return 1;
}

void IffSkipCurrentChunk(IffFile *iff, const IffChunk *chunk)
{
  const DataSize next_pos = chunk->data_head + chunk->data_size;
  iff_seek(iff, next_pos, IFF_SEEK_SET);
}

int IffChunkMatch(const IffChunk *chunk, const char *key)
{
  return strcmp(chunk->id, key) == 0 ? 1 : 0;
}

int IffEndOfChunk(const IffFile *iff, const IffChunk *chunk)
{
  const DataSize end_pos = chunk->data_head + chunk->data_size;
  DataSize cur_pos = iff_tell(iff);

  if (cur_pos % 2 == 1) {
    cur_pos++;
  }

  if (end_pos == cur_pos) {
    return 1;
  } else {
    return 0;
  }
}
