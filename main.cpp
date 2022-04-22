#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>

using namespace std;

#include "parameters.h"
#include "tools.h"
#include "type_aliases.h"

int main()
{
   // Create a set of all words and a set of only words that are allowed
   // to be answers
   word_list_t all_words;
   word_list_t answers;

   load_words(all_words, answers);

   // guess --> entropy
   entropy_words_map_t entropies;

   chrono::time_point<chrono::steady_clock> start_time;
   chrono::time_point<chrono::steady_clock> stop_time;

   if constexpr (DEBUG_MODE)
      start_time = chrono::steady_clock::now();

   calculate_entropies(all_words, answers, entropies);

   if constexpr (DEBUG_MODE)
      stop_time = chrono::steady_clock::now();

   if constexpr (DEBUG_MODE)
      print_entropies(entropies);

   if constexpr (DEBUG_MODE)
   {
      auto ticks_taken{stop_time - start_time};
      constexpr long double tick_interval{decltype(ticks_taken)::period::den};
      long double time_taken{static_cast<long double>(ticks_taken.count()) / tick_interval};

      cout << endl;

      cout << "Time taken by program is: " << fixed
           << setprecision(6) << time_taken << " seconds" << endl;
   }

   cout << endl;

   // Set up regular expressions to test validity of inputs
   stringstream word_ss;
   word_ss << "[a-z]{" << WORD_LENGTH << "}";
   const regex word_regex(word_ss.str());

   stringstream result_ss;
   result_ss << "[byg]{" << WORD_LENGTH << "}";
   const regex result_regex(result_ss.str());

   // Set up variables to keep track of what we learn about
   // the answer's letters and their positions.
   set<char> unused_letters;
   vector<set<char>> location_unknown_letters(WORD_LENGTH);
   vector<char> known_letters(WORD_LENGTH, '\0');

   // Proceed with the program's main loop
   for (my_uint_t round{0}; round < 6; ++round)
   {
      cout << "Round " << round + 1 << endl;

      string guess;
      string result;

      while (true)
      {
         smatch m;

         cout << "Word: ";
         cin >> guess;

         if (! regex_match(guess, m, word_regex))
         {
            cout << endl;
            cout << "Invalid: " << guess << endl << endl;

            continue;
         }

         if (all_words.find(guess) == answers.end())
            cout << "Not a word!" << endl << endl;
         else
            break;
      }

      while (true)
      {
         smatch m;

         cout << "Result: ";
         cin >> result;

         if (! regex_match(result, m, result_regex))
         {
            cout << endl;
            cout << "Invalid: " << result << endl << endl;

            continue;
         }
         else
            break;
      }

      if (result == "ggggg")
         break;

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
               for (const char one_char : one_set)
               {
                  if (one_char == c)
                  {
                     ok_to_mark_unused = false;
                     break;
                  }
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

      if constexpr (DEBUG_MODE)
         cout << "regex: " << search_regex_ss.str() << endl;

      const regex candidate_regex(search_regex_ss.str());
      set<string> candidate_answers;

      for (const string &word : answers)
      {
         smatch m;

         if (regex_match(word, m, candidate_regex))
            candidate_answers.insert(word);
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

      if constexpr (DEBUG_MODE)
         cout << "Letters to be placed: ";

      for (char c : all_location_unknown_letters)
      {
         if constexpr (DEBUG_MODE)
            cout << c;

         for (
                auto iter {candidate_answers.begin()};
                iter != candidate_answers.end();
            )
         {
            if (iter->find(c) == string::npos)
               iter = candidate_answers.erase(iter);
            else
               ++iter;
         }
      }

      if constexpr (DEBUG_MODE)
      {
         cout << endl << endl;

         for (const string &word : candidate_answers)
            cout << word << endl;
      }

      cout << endl;
   }
}
