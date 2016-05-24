#ifndef __VTS_SYSFUZZER_LIBMEASUREMENT_GCDA_PARSER_H__
#define __VTS_SYSFUZZER_LIBMEASUREMENT_GCDA_PARSER_H__

#include <vector>

using namespace std;

namespace android {
namespace vts {

typedef struct tag_format {
  unsigned tag;
  char const* name;
  void (*proc)(const char*, unsigned, unsigned, vector<unsigned>*);
} tag_format_t;

void tag_counters(const char* filename, unsigned tag, unsigned length, vector<unsigned>* result);
void tag_function(const char* filename, unsigned tag, unsigned length, vector<unsigned>* result);
void tag_blocks(const char* filename, unsigned tag, unsigned length, vector<unsigned>* result);
void tag_arcs(const char* filename, unsigned tag, unsigned length, vector<unsigned>* result);
void tag_lines(const char* filename, unsigned tag, unsigned length, vector<unsigned>* result);

vector<unsigned>* parse_gcda_file(const char* filename);

}  // namespace vts
}  // namespace android

#endif
