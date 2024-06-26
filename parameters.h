#ifndef PARAMETERS_INCLUDED
#define PARAMETERS_INCLUDED

#include <cstddef>
#include <limits>

using namespace std;

#include "type_aliases.h"

constexpr my_uint_t ROUNDS{6};
constexpr my_uint_t WORD_LENGTH{5};

// Specify whether the recommended guess should be used automatically
// or if the user should be prompted to enter a guess. This is useful
// when solving multiple puzzles simultaneously.
constexpr bool MANUAL_MODE{false};

// This allows the user to set a limit on the number of threads.
constexpr my_uint_t THREADS_LIMIT{numeric_limits<my_uint_t>::max()};
// constexpr my_uint_t THREADS_LIMIT{<your desired value>};

// Save filtered word lists to disk after each round
constexpr bool DEBUG_MODE{false};

// To save a few seconds at program start, a hard-coded first
// guess can be used.
constexpr bool USE_HARDCODED_FIRST_GUESS{true};

const string allowed_guesses_filename{"wordle-allowed-guesses.txt"};
const string allowed_answers_filename{"wordle-answers-alphabetical.txt"};
// const string allowed_guesses_filename{"wordmaster-allowed-guesses.txt"};
// const string allowed_answers_filename{"wordmaster-answers-alphabetical.txt"};

#endif
