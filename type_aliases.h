#ifndef TYPE_ALIASES_INCLUDED
#define TYPE_ALIASES_INCLUDED

#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;

using entropy_t = long double;
using bin_item_count_map_t = unordered_map<string, size_t>;
using bin_probability_map_t = unordered_map<string, entropy_t>;
using entropy_words_map_t = multimap<entropy_t, string, greater<entropy_t>>;
using word_list_t = unordered_set<string>;

#endif
