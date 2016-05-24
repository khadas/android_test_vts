#include "gcda_parser.h"

#include <vector>
#include <iostream>

#include "gcov_basic_io.h"

using namespace std;

namespace android {
namespace vts {

static const tag_format_t tag_table[] = {
  {0, "NOP", NULL},
  {0, "UNKNOWN", NULL},
  {0, "COUNTERS", tag_counters},
  {GCOV_TAG_FUNCTION, "FUNCTION", tag_function},
  {GCOV_TAG_BLOCKS, "BLOCKS", tag_blocks},
  {GCOV_TAG_ARCS, "ARCS", tag_arcs},
  {GCOV_TAG_LINES, "LINES", tag_lines},
  {0, NULL, NULL}
};


void tag_counters(const char* filename, unsigned tag, unsigned length,
                  vector<unsigned>* result) {
  unsigned n_counts = GCOV_TAG_COUNTER_NUM (length);
  printf("%s: %d counts\n", __FUNCTION__, n_counts);
}


void tag_function(const char* filename, unsigned tag, unsigned length,
                  vector<unsigned>* result) {
  unsigned long pos = gcov_position();

  if (length) {
    gcov_read_unsigned();  // ident %u
    unsigned lineno_checksum = gcov_read_unsigned();
    result->push_back(lineno_checksum);
    gcov_read_unsigned();  // cfg_checksum 0x%08x
  }
}


void tag_blocks(const char* filename, unsigned tag, unsigned length,
                vector<unsigned>* result) {
  unsigned n_blocks = GCOV_TAG_BLOCKS_NUM(length);
  printf("%s: %u blocks\n", __FUNCTION__, n_blocks);
}


void tag_arcs(const char* filename, unsigned tag, unsigned length,
              vector<unsigned>* result) {
  unsigned n_arcs = GCOV_TAG_ARCS_NUM(length);
  printf("%s: %u arcs\n", __FUNCTION__, n_arcs);
}


void tag_lines(const char* filename, unsigned tag, unsigned length,
               vector<unsigned>* result) {
  printf("%s\n", __FUNCTION__);
}


vector<unsigned>* parse_gcda_file(const char* filename) {
  unsigned tags[4];
  unsigned depth = 0;
  vector<unsigned>* result;

  result = new vector<unsigned>();
  if (!gcov_open(filename, 1)) {
    fprintf(stderr, "%s:cannot open\n", filename);
    return result;
  }

  /* magic */
  {
    unsigned magic = gcov_read_unsigned ();
    unsigned version;
    const char* type = NULL;
    int endianness = 0;
    char m[4], v[4];

    if ((endianness = gcov_magic (magic, GCOV_DATA_MAGIC))) {
      type = "data";
    } else {
      printf ("%s:not a gcov file\n", filename);
      gcov_close();
      return result;
    }
    version = gcov_read_unsigned();
    GCOV_UNSIGNED2STRING(v, version);
    GCOV_UNSIGNED2STRING(m, magic);
    if (version != GCOV_VERSION) {
      char e[4];
      GCOV_UNSIGNED2STRING (e, GCOV_VERSION);
    }
  }

  gcov_read_unsigned();  // stamp
  int cnt = 0;
  bool found;
  while (1) {
    unsigned base, position = gcov_position();
    unsigned tag, length;
    tag_format_t const* format;
    unsigned tag_depth;
    int error;
    unsigned mask;

    tag = gcov_read_unsigned();
    if (!tag)
      break;

    length = gcov_read_unsigned();
    base = gcov_position();
    mask = GCOV_TAG_MASK(tag) >> 1;
    for (tag_depth = 4; mask; mask >>= 8) {
      if ((mask & 0xff) != 0xff) {
        printf ("%s:tag `%08x' is invalid\n", filename, tag);
        break;
      }
      tag_depth--;
    }
    found = false;
    for (format = tag_table; format->name; format++) {
      if (format->tag == tag) {
        found = true;
        break;
      }
    }
    if (!found) format = &tag_table[GCOV_TAG_IS_COUNTER (tag) ? 2 : 1];

    if (tag) {
      if (depth && depth < tag_depth) {
        if (!GCOV_TAG_IS_SUBTAG (tags[depth - 1], tag))
          printf ("%s:tag `%08x' is incorrectly nested\n",
                  filename, tag);
      }
      depth = tag_depth;
      tags[depth - 1] = tag;
    }
    if (format->proc) {
      (*format->proc) (filename, tag, length, result);
    }
    gcov_sync (base, length);
    if ((error = gcov_is_error ())) {
      printf(error < 0 ? "%s:counter overflow at %lu\n" :
             "%s:read error at %lu\n", filename,
             (long unsigned) gcov_position());
      break;
    }
  }
  gcov_close();
  return result;
}

}  // namespace vts
}  // namespace android

