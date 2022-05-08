#include <iostream>
#include <sstream>
#include <string>

using namespace std;

#include "filter.h"
#include "parameters.h"
#include "tools.h"
#include "type_aliases.h"

int main(int argc, char *argv[])
{
   const string target_word{argc == 2 ? argv[1] : ""};

   if (argc > 2)
   {
      cout << "Usage: " << argv[0] << " [<target word>]" << endl;
      cout << "   If a target word is supplied, result calculations" << endl;
      cout << "   will be performed automatically. Otherwise, the" << endl;
      cout << "   user will have to enter them manually." << endl;

      cout << endl;

      return 255;
   }

   // Load word lists into memory. This is done in a quite space-inefficient
   // way, but it doesn't harm anything, so leave it since it's the clearest
   // way to maintain these lists.

   // Create a set of all words.
   word_list_t all_words_unfiltered;

   // Create a set of all words that are allowed answers. This list
   // gets filtered down as the game proceeds.
   word_list_t answers_filtered;

   // Read these two lists of words from disk.
   load_words(all_words_unfiltered, answers_filtered);

   // Ensure the target_word, if user-supplied, is in the list of allowed answers
   if (target_word != "")
   {
      if (answers_filtered.find(target_word) == answers_filtered.cend())
      {
         cout << "The supplied target word, "
              << target_word
              << ", is not an allowed answer word!"
              << endl;

         cout << endl;

         return 255;
      }
   }

   // Set up a regular expression to test validity of result inputs
   stringstream result_ss;
   result_ss << "[byg]{" << WORD_LENGTH << "}";
   const regex result_regex(result_ss.str());

   // Proceed with the program's main loop
   filter_t filter;
   my_uint_t round{1};

   for (; round <= ROUNDS; ++round)
   {
      cout << "Round " << round << endl;

      // If debugging, save the list of possible answers to disk.
      if constexpr (DEBUG_MODE)
      {
         stringstream ss;

         ss << "answers_filtered_" << round << ".txt";
         save_word_list(answers_filtered, ss.str());
      }

      // Determine the next guess
      string guess;

      get_guess(all_words_unfiltered, answers_filtered, round, guess);

      // Get the result of the user's guess
      string result;

      if (target_word != "")
      {
         result = compare(target_word, guess);
         cout << result << endl;
      }
      else
         get_user_input("Result", result_regex, result);

      if (result == "ggggg")
         break;

      // Filter the list of possible answers
      filter.filter(answers_filtered, guess, result);

      // Remove the guessed word from our word lists
      all_words_unfiltered.erase(guess);
      answers_filtered.erase(guess);

      cout << endl;
   }

   if (round == ROUNDS + 1)
   {
      cout << "Could not solve the puzzle!" << endl;
      cout << endl;

      return 254;
   }

   return round;
}
