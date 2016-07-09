#include "gcov_basic_io.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace android {
namespace vts {

struct gcov_var_t gcov_var;

unsigned gcov_position(void) { return gcov_var.start + gcov_var.offset; }

int gcov_is_error() { return gcov_var.file ? gcov_var.error : 1; }

static inline unsigned from_file(unsigned value) {
  if (gcov_var.endian) {
    value = (value >> 16) | (value << 16);
    value = ((value & 0xff00ff) << 8) | ((value >> 8) & 0xff00ff);
  }
  return value;
}

unsigned gcov_read_string_array(char** string_array, unsigned num_strings) {
  unsigned i, j, len = 0;

  for (j = 0; j < num_strings; j++) {
    unsigned string_len = gcov_read_unsigned();
    string_array[j] = (char*)malloc(string_len * sizeof(unsigned));  // xmalloc
    for (i = 0; i < string_len; i++)
      ((unsigned*)string_array[j])[i] = gcov_read_unsigned();
    len += (string_len + 1);
  }
  return len;
}

unsigned gcov_read_unsigned() {
  unsigned value;
  const unsigned* buffer = gcov_read_words(1);

  if (!buffer) return 0;
  value = from_file(buffer[0]);
  return value;
}

void gcov_allocate(unsigned length) {
  size_t new_size = gcov_var.alloc;

  if (!new_size) new_size = GCOV_BLOCK_SIZE;
  new_size += length;
  new_size *= 2;
  gcov_var.alloc = new_size;
  gcov_var.buffer = (unsigned*)realloc(gcov_var.buffer, new_size << 2);
}

const unsigned* gcov_read_words(unsigned words) {
  const unsigned* result;
  unsigned excess = gcov_var.length - gcov_var.offset;

  assert(gcov_var.mode > 0);
  if (excess < words) {
    gcov_var.start += gcov_var.offset;
    memmove(gcov_var.buffer, gcov_var.buffer + gcov_var.offset, excess * 4);
    gcov_var.offset = 0;
    gcov_var.length = excess;
    if (gcov_var.length + words > gcov_var.alloc) {
      gcov_allocate(gcov_var.length + words);
    }
    excess = gcov_var.alloc - gcov_var.length;
    excess = fread(gcov_var.buffer + gcov_var.length, 1, excess << 2,
                   gcov_var.file) >>
             2;
    gcov_var.length += excess;
    if (gcov_var.length < words) {
      gcov_var.overread += words - gcov_var.length;
      gcov_var.length = 0;
      return 0;
    }
  }
  result = &gcov_var.buffer[gcov_var.offset];
  gcov_var.offset += words;
  return result;
}

int gcov_magic(unsigned magic, unsigned expected) {
  if (magic == expected) return 1;
  magic = (magic >> 16) | (magic << 16);
  magic = ((magic & 0xff00ff) << 8) | ((magic >> 8) & 0xff00ff);
  if (magic == expected) {
    gcov_var.endian = 1;
    return -1;
  }
  return 0;
}

gcov_type gcov_read_counter() {
  gcov_type value;
  const unsigned* buffer = gcov_read_words(2);
  if (!buffer) return 0;
  value = from_file(buffer[0]);
  if (sizeof(value) > sizeof(unsigned)) {
    value |= ((gcov_type)from_file(buffer[1])) << 32;
  } else if (buffer[1]) {
    gcov_var.error = -1;
  }
  return value;
}

void gcov_write_block(unsigned size) {
  if (fwrite(gcov_var.buffer, size << 2, 1, gcov_var.file) != 1) {
    gcov_var.error = 1;
  }
  gcov_var.start += size;
  gcov_var.offset -= size;
}

const char* gcov_read_string() {
  unsigned length = gcov_read_unsigned();
  if (!length) return 0;
  return (const char*)gcov_read_words(length);
}

bool gcov_open(const char* name, int mode) {
  assert(!gcov_var.file);

  gcov_var.start = 0;
  gcov_var.offset = 0;
  gcov_var.length = 0;
  gcov_var.overread = -1u;
  gcov_var.error = 0;
  gcov_var.endian = 0;

  if (mode >= 0) {
    gcov_var.file = fopen(name, (mode > 0) ? "rb" : "r+b");
  }

  if (gcov_var.file) {
    gcov_var.mode = 1;
  } else if (mode <= 0) {
    gcov_var.file = fopen(name, "w+b");
    if (gcov_var.file) {
      gcov_var.mode = mode * 2 + 1;
    }
  }
  if (!gcov_var.file) return false;

  setbuf(gcov_var.file, (char*)0);
  return true;
}

void gcov_sync(unsigned base, unsigned length) {
  assert(gcov_var.mode > 0);
  base += length;
  if (base - gcov_var.start <= gcov_var.length) {
    gcov_var.offset = base - gcov_var.start;
  } else {
    gcov_var.offset = gcov_var.length = 0;
    fseek(gcov_var.file, base << 2, SEEK_SET);
    gcov_var.start = ftell(gcov_var.file) >> 2;
  }
}

int gcov_close() {
  if (gcov_var.file) {
    if (gcov_var.offset && gcov_var.mode < 0) {
      gcov_write_block(gcov_var.offset);
    }
    fclose(gcov_var.file);
    gcov_var.file = 0;
    gcov_var.length = 0;
  }
  free(gcov_var.buffer);
  gcov_var.alloc = 0;
  gcov_var.buffer = 0;
  gcov_var.mode = 0;
  return gcov_var.error;
}

}  // namespace vts
}  // namespace android
