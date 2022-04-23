#ifndef PARAMETERS_INCLUDED
#define PARAMETERS_INCLUDED

#include <cstddef>
#include <limits>

using namespace std;

#include "type_aliases.h"

constexpr my_uint_t WORD_LENGTH{5};

// This allows the user to set a limit on the number of threads.
constexpr my_uint_t THREADS_LIMIT{numeric_limits<my_uint_t>::max()};
// constexpr my_uint_t THREADS_LIMIT{<your desired value>};

#endif
