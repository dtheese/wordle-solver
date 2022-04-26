#ifndef FILTER_INCLUDED
#define FILTER_INCLUDED

#include <set>
#include <string>
#include <vector>

using namespace std;

#include "type_aliases.h"

class filter_t
{
   public:
      filter_t();

      void filter(
                    word_list_t &answers_filtered,
                    const string &guess,
                    const string &result
                 );

   private:
      // Set up member variables to keep track of what we learn about
      // the answer's letters and their positions.
      set<char> unused_letters;
      vector<set<char>> location_unknown_letters;
      vector<char> known_letters;
};

#endif
