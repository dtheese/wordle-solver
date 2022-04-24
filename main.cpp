#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <regex>
#include <set>
#include <sstream>
#include <string>

using namespace std;

#include "parameters.h"
#include "tools.h"
#include "type_aliases.h"

int main(int argc, char *argv[])
{
   string target_word;

   if (argc > 2)
   {
      cout << "Usage: " << argv[0] << " [<target word>]" << endl;
      cout << "   If a target word is supplied, result calculations" << endl;
      cout << "   will be performed automatically. Otherwise, the" << endl;
      cout << "   user will have to enter them manually." << endl;

      cout << endl;

      return 1;
   }
   else if (argc == 2)
      target_word = argv[1];

   // Load word lists into memory. This is done in a quite space-inefficient
   // way, but it doesn't harm anything, so leave it since it's the clearest
   // way to maintain these lists.

   // Create a set of all words (used to enforce that guesses be valid words).
   // Create a set of all words that are allowed answers.
   word_list_t all_words_unfiltered;
   word_list_t answers_unfiltered;

   load_words(all_words_unfiltered, answers_unfiltered);

   // Ensure the target_word, if user-supplied, is in the corpus
   if (target_word != "")
   {
      if (all_words_unfiltered.find(target_word) == all_words_unfiltered.cend())
      {
         cout << "The supplied target word, "
              << target_word
              << ", is not a word!"
              << endl;

         cout << endl;

         return 1;
      }
   }

   // Create a set of all words that haven't been filtered out by the game's results.
   // This is initially the set of all words.
   word_list_t all_words_filtered{all_words_unfiltered};

   // Create a set of allowed answers that haven't been filtered out by the
   // game's results. This is initially the set of all allowed answers.
   word_list_t answers_filtered{answers_unfiltered};

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
   for (my_uint_t round{0}; round < ROUNDS; ++round)
   {
      cout << "Round " << round + 1 << endl;

      // entropy --> word(s) with that entropy
      entropy_words_map_t entropies;

      calculate_entropies(all_words_unfiltered, answers_filtered, entropies);

      // Determine the next guess
      string guess;

      word_list_t intersection;

      set_intersection(
                         all_words_filtered.cbegin(),
                         all_words_filtered.cend(),
                         answers_filtered.cbegin(),
                         answers_filtered.cend(),
                         inserter(intersection, intersection.end())
                      );

      if (intersection.size() == 1)
      {
         guess = *(intersection.cbegin());

         cout << "Only remaining allowed answer word: "
              << guess
              << endl;
      }
      else
      {
         guess = entropies.cbegin()->second;

         cout << "Best guess by entropy: "
              << guess
              << " ("
              << entropies.cbegin()->first
              << ")"
              << endl;
      }

      if constexpr (MANUAL_MODE)
      {
#if 0
         stringstream ss;

         ss << "all_words_filtered_" << round + 1 << ".txt";
         save_word_list(all_words_filtered, ss.str());

         ss.str("");
         ss << "answers_filtered_" << round + 1 << ".txt";
         save_word_list(answers_filtered, ss.str());
#endif

         while (true)
         {
            get_user_input("Word", word_regex, guess);

            if (all_words_unfiltered.find(guess) == all_words_unfiltered.cend())
               cout << "Not a word!" << endl << endl;
            else
               break;
         }
      }

      // Get the result of the user's guess
      string result;

      if (target_word != "")
         result = compare(target_word, guess);
      else
         get_user_input("Result", result_regex, result);

      cout << result << endl;

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

      const regex re(search_regex_ss.str());

      // Use the regular expression to filter the guess list.
      // More filtering will be done later.
      for (
             auto iter{all_words_filtered.cbegin()};
             iter != all_words_filtered.cend();
          )
      {
         smatch m;

         if (! regex_match(*iter, m, re))
            iter = all_words_filtered.erase(iter);
         else
            ++iter;
      }

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
                auto iter {all_words_filtered.cbegin()};
                iter != all_words_filtered.cend();
            )
         {
            if (iter->find(c) == string::npos)
               iter = all_words_filtered.erase(iter);
            else
               ++iter;
         }

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

      cout << endl;
   }
}
