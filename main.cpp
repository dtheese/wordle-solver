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

   // Ensure the target_word, if user-supplied, is in the list of allowed anwers
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

   // Set up regular expressions to test validity of inputs
   stringstream word_ss;
   word_ss << "[a-z]{" << WORD_LENGTH << "}";
   const regex word_regex(word_ss.str());

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

      if (answers_filtered.size() == 0)
      {
         cout << "No possible answer words remain. "
              << "Something is wrong!"
              << endl;

         return 255;
      }
      else if (answers_filtered.size() == 1)
      {
         guess = *(answers_filtered.cbegin());

         cout << "Only remaining allowed answer word: "
              << guess
              << endl;
      }
      else if (answers_filtered.size() <= (ROUNDS - round + 1))
      {
         // This is the case where the number of possible answers
         // remaining is less than or equal to the number of guesses
         // remaining. We are guaranteed a win. To pick which word
         // to try next, again use entropy, but only over the possible
         // answers (as opposed to the whole corpus). I still need to
         // test if this yields any actual improvement, but it can't
         // hurt since we are guaranteed a win at this point.

         // entropy --> word(s) with that entropy
         entropy_words_map_t entropies;

         calculate_entropies(answers_filtered, answers_filtered, entropies);

         guess = entropies.cbegin()->second;

         cout << "Best guess by entropy over "
              << answers_filtered.size()
              << " remaining possible answers: "
              << guess
              << " ("
              << entropies.cbegin()->first
              << ")"
              << endl;
      }
      else
      {
         // entropy --> word(s) with that entropy
         entropy_words_map_t entropies;

         if (round > 1)
            calculate_entropies(all_words_unfiltered, answers_filtered, entropies);
         else
            entropies.insert({1.49, "soare"});

         guess = entropies.cbegin()->second;

         cout << "Best guess by entropy taken over all guess words: "
              << guess
              << " ("
              << entropies.cbegin()->first
              << ")"
              << endl;
      }

      // Let the user manually input the guess if that's what they want.
      // This is useful when solving mutiple puzzles simultaneously.
      // If MANUAL_MODE = false, just use the suggested guess automatically.
      if constexpr (MANUAL_MODE)
      {
         while (true)
         {
            get_user_input("Word", word_regex, guess);

            if (all_words_unfiltered.find(guess) == all_words_unfiltered.cend())
               cout << "Not a valid guess!" << endl << endl;
            else
               break;
         }
      }

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
