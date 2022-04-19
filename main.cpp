#include <cassert>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>

using namespace std;

#include "tools.h"

int main(int argc, char *argv[])
{
   // TODO: Remove void cast of argc and argv
   (void) argc;
   (void) argv;

   constexpr bool DEBUG_MODE{false};
   constexpr bool GREEN_CHECK{false};
   constexpr size_t WORD_LENGTH{5};

   // Load allowed guesses (and all_words)
   set<string> all_words;
   set<string> allowed_guesses;
   set<string> answers;

   // Load possible guesses (and all_words)
   {
      const string allowed_guesses_filename{"wordle-allowed-guesses.txt"};
      ifstream words(allowed_guesses_filename);
   
      if (! words)
      {
         stringstream ss;
   
         ss << allowed_guesses_filename << " is missing";
         throw runtime_error(ss.str());
      }

      string word;

      while (getline(words, word))
      {
         allowed_guesses.insert(word);
         all_words.insert(word);
      }

      words.close();
   }

   // Load possible answers (and all_words)
   {
      const string answers_filename{"wordle-answers-alphabetical.txt"};
      ifstream words(answers_filename);

      if (! words)
      {
         stringstream ss;

         ss << answers_filename << " is missing";
         throw runtime_error(ss.str());
      }

      string word;

      while (getline(words, word))
      {
         answers.insert(word);
         all_words.insert(word);
      }

      words.close();
   }

   // Set up regular expressions to test validity of inputs
   stringstream word_ss;
   word_ss << "[a-z]{" << WORD_LENGTH << "}";
   const regex word_regex(word_ss.str());

   stringstream result_ss;
   result_ss << "[byg]{" << WORD_LENGTH << "}";
   const regex result_regex(result_ss.str());

   // Set up variables to keep track of what we learn about
   // the answer's letters their positions.
   set<char> unused_letters;
   vector<set<char>> location_unknown_letters(WORD_LENGTH);
   vector<char> known_letters(WORD_LENGTH, '\0');

   // Proceed with the program's main loop
   for (size_t round{0}; round < 6; ++round)
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

      for (size_t i{0}; i < WORD_LENGTH; ++i)
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

      for (size_t i{0}; i < WORD_LENGTH; ++i)
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

            if constexpr (GREEN_CHECK)
            {
               // I now believe this check to be unneeded, and have
               // noted that it can leave this list of candidate words
               // larger than it needs to be.
               // 
               // EXAMPLE
               // -------
               // If flair is the answer:
               // 
               // Round 1: slate --> bggbb
               // [^est]la[^est][^est]
               // 
               // Round 2: blank --> bggbb
               // [^beknst]la[^beknst][^beknst]
               // 
               // Round 3: plaid --> bgggb
               // [^bdeknpst]lai[^bdeknpst]
               // 
               // Round 4: flail --> ggggb
               // flai[^bdeknpst]
               // 
               // After round 4, the word flail is still in the list of possible answers.
   
               // Check to see if this letter is green in *any* other
               // position before marking it unused!
               if (
                     find(
                            known_letters.cbegin(),
                            known_letters.cend(),
                            c
                         ) != known_letters.cend()
                  )
               {
   
                  continue;
               }
            }

            unused_letters.insert(c);
         }
      }

      // Build a regular expression to search for words that meet the criteria
      // of the letters marked green and to ensure the letters marked yellow
      // don't appear *in the position they were guessed at*.
      stringstream search_regex_ss;

      for (size_t i{0}; i < WORD_LENGTH; ++i)
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

      for (size_t i{0}; i < WORD_LENGTH; ++i)
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
