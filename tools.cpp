#include <cassert>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

using namespace std;

#include "parameters.h"
#include "tools.h"

void calculate_entropies(
                           const word_list_t &all_words,
                           const word_list_t &answers,
                           entropy_words_map_t &entropies
                        )
{
   entropies.clear();

   const entropy_t total_item_count(all_words.size());

   for (const string &guess : all_words)
   {
      // bin --> item count in bin
      bin_item_count_map_t bins;

      for (const string &answer : answers)
         ++bins[compare(answer, guess)];

      // bin --> probability of landing in bin
      bin_probability_map_t probabilities;

      for (const auto &[bin, item_count] : bins)
         probabilities[bin] = item_count / total_item_count;

      entropy_t entropy{};

      for (const auto &[word, prob] : probabilities)
         entropy -= prob * log2(prob);

      entropies.insert({entropy, guess});
   }
}

string compare(const string &answer, const string &guess)
{
   assert(answer.size() == WORD_LENGTH);
   assert(guess.size() == WORD_LENGTH);

   string rval{"*****"};

   unordered_map<char, size_t> char_count_in_answer{};
   unordered_map<char, size_t> char_count_in_guess{};
   unordered_map<char, size_t> char_count_marked{};

   // Get count of characters in answer and guess
   for (size_t i{0}; i < WORD_LENGTH; ++i)
   {
      ++char_count_in_answer[answer[i]];
      ++char_count_in_guess[guess[i]];
   }

   // Mark green squares
   for (size_t i{0}; i < WORD_LENGTH; ++i)
   {
      if (guess[i] == answer[i])
      {
         rval[i] = 'g';
         ++char_count_marked[guess[i]];
      }
   }

   // Mark squares that are definitely black
   for (size_t i{0}; i < WORD_LENGTH; ++i)
   {
      if (char_count_in_answer[guess[i]] == 0)
         rval[i] = 'b';
   }

   // Mark yellow squares and remaining black squares. This can be tricky.
   // Consider this case (with made up words):
   // Answer            : abcda
   // Guess             : azaza
   // Correct response  : gbbbg
   // Incorrect response: gbybg
   while (rval.find('*') != string::npos)
   {
      for (size_t i{0}; i < WORD_LENGTH; ++i)
      {
         if (rval[i] != '*')
            continue;

         char guess_char{guess[i]};
         size_t num_guess_char_marked_so_far{char_count_marked[guess_char]};
         size_t num_guess_char_to_be_marked{char_count_in_answer[guess_char]};

         if (num_guess_char_to_be_marked > num_guess_char_marked_so_far)
         {
            rval[i] = 'y';
            ++char_count_marked[guess_char];
         }
         else
            rval[i] = 'b';
      }
   }

   return rval;
}

void load_words(word_list_t &all_words, word_list_t &answers)
{
   all_words.clear();
   answers.clear();

   // Load allowed guesses which aren't possible answers into
   // the list of all words.
   const string allowed_guesses_filename{"wordle-allowed-guesses.txt"};
   ifstream allowed_guesses(allowed_guesses_filename);

   if (! allowed_guesses)
   {
      stringstream ss;

      ss << allowed_guesses_filename << " is missing";
      throw runtime_error(ss.str());
   }

   string guess;

   while (getline(allowed_guesses, guess))
      all_words.insert(guess);

   allowed_guesses.close();

   // Now load possible answers into its own list as well as
   // into the list of all words.
   const string allowed_answers_filename{"wordle-answers-alphabetical.txt"};
   ifstream allowed_answers(allowed_answers_filename);

   if (! allowed_answers)
   {
      stringstream ss;

      ss << allowed_answers_filename << " is missing";
      throw runtime_error(ss.str());
   }

   string answer;

   while (getline(allowed_answers, answer))
   {
      answers.insert(answer);
      all_words.insert(answer);
   }

   allowed_answers.close();
}

void print_entropies(const entropy_words_map_t &entropies)
{
   for (const auto &[entropy, word] : entropies)
   {
      cout << fixed
           << setprecision(numeric_limits<entropy_t>::digits)
           << word
           << ": "
           << entropy
           << endl;
   }
}
