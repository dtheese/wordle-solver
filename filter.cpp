#include <cassert>
#include <algorithm>
#include <regex>
#include <set>

using namespace std;

#include "parameters.h"

#include "filter.h"

filter_t::filter_t(): location_unknown_letters(WORD_LENGTH),
                      known_letters(WORD_LENGTH, '\0')
{
}

void filter_t::filter(
                        word_list_t &answers_filtered,
                        const string &guess,
                        const string &result
                     )
{
   // Build a list of unused letters, location unknown letters,
   // and location known letters

   // Be careful here! If the answer has N instances of a letter
   // and if we make a guess that has more than N instances of that
   // letter, N instances will be reported as yellow or green, but
   // the remaining instances will be reported as black. We should
   // not let that cause us to erroneously add that letter to the
   // set of unused letters! Furthermore, since we don't have a
   // guarantee of *which* N instances will be reported as yellow
   // or green, we must defer the processing of black results until
   // after all yellow and green results have been processed.

   for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
   {
      char c{guess[i]};

      if (result[i] == 'b')
         continue;
      else if (result[i] == 'y')
         location_unknown_letters[i].insert(c);
      else if (result[i] == 'g')
         known_letters[i] = c;
      else
         assert(0);
   }

   for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
   {
      if (result[i] == 'b')
      {
         char c{guess[i]};

         // Check to see if this letter is yellow in *any* other
         // position before leaving it marked as unused!
         bool ok_to_mark_unused{true};

         for (const auto &one_set : location_unknown_letters)
         {
            if (
                  any_of(
                           one_set.cbegin(),
                           one_set.end(),
                           [=](char one_char){ return one_char == c; }
                        )
               )
            {
               ok_to_mark_unused = false;
            }

            if (! ok_to_mark_unused)
               break;
         }

         if (! ok_to_mark_unused)
            continue;

         unused_letters.insert(c);
      }
   }

   // Build a regular expression to search for words that meet the criteria
   // of the letters marked green and to ensure the letters marked yellow
   // don't appear *in the position they were guessed at*.
   stringstream search_regex_ss;

   for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
   {
      if (known_letters[i] == '\0')
      {
         search_regex_ss << "[^";

         for (char c : unused_letters)
            search_regex_ss << c;

         for (char c : location_unknown_letters[i])
            search_regex_ss << c;

         search_regex_ss << "]";
      }
      else
         search_regex_ss << known_letters[i];
   }

   const regex re(search_regex_ss.str());

   // Use the regular expression to filter the possible answer list.
   // More filtering will be done later.
   for (
          auto iter{answers_filtered.cbegin()};
          iter != answers_filtered.cend();
       )
   {
      smatch m;

      if (! regex_match(*iter, m, re))
         iter = answers_filtered.erase(iter);
      else
         ++iter;
   }

   // Ensure that letters that must be present are present
   set<char> all_location_unknown_letters;

   for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
   {
      for (char c : location_unknown_letters[i])
      {
         if (
               find(
                      known_letters.cbegin(),
                      known_letters.cend(),
                      c
                   ) ==
               known_letters.cend()
            )
         {
            all_location_unknown_letters.insert(c);
         }
      }
   }

   for (char c : all_location_unknown_letters)
   {
      for (
             auto iter {answers_filtered.cbegin()};
             iter != answers_filtered.cend();
         )
      {
         if (iter->find(c) == string::npos)
            iter = answers_filtered.erase(iter);
         else
            ++iter;
      }
   }
}
