#ifndef TYPE_ALIASES_INCLUDED
#define TYPE_ALIASES_INCLUDED

#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <set>
#include <string>

using namespace std;

using my_uint_t = long unsigned int;
using entropy_t = long double;
using bin_item_count_map_t = map<string, my_uint_t>;
using bin_probability_map_t = map<string, entropy_t>;
using entropy_words_map_t = multimap<entropy_t, string, greater<entropy_t>>;
using word_list_t = set<string>;

#endif
