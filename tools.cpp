#include <cassert>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;

#include "parameters.h"
#include "print_mutex.h"
#include "tools.h"

namespace
{
   const my_uint_t MAX_HW_THREADS{thread::hardware_concurrency()};

   const my_uint_t NUM_THREADS{
                                 min(
                                       {
                                          THREADS_LIMIT,
                                          MAX_HW_THREADS > 0 ? MAX_HW_THREADS : 1
                                       }
                                    )
                              };

   entropy_words_map_t iterate_over_subset_of_words(
                                                      word_list_t::const_iterator first,
                                                      word_list_t::const_iterator last,
                                                      const word_list_t &answers,
                                                      my_uint_t total_item_count
                                                   );
}

void calculate_entropies(
                           const word_list_t &all_words,
                           const word_list_t &answers,
                           entropy_words_map_t &entropies
                        )
{
   entropies.clear();

   const my_uint_t total_item_count(all_words.size());
   vector<future<entropy_words_map_t>> futures;
   const my_uint_t guesses_per_thread{total_item_count / NUM_THREADS};

   {
      lock_guard<mutex> lg{print_mutex};

      for (my_uint_t i{0}; i < NUM_THREADS; ++i)
      {
         word_list_t::const_iterator first{all_words.cbegin()};
         advance(first, i * guesses_per_thread);

         word_list_t::const_iterator last{first};
         advance(last, guesses_per_thread);

         if (i == (NUM_THREADS - 1))
            advance(last, total_item_count % NUM_THREADS);

         futures.push_back(
                             async(
                                     launch::async,
                                     iterate_over_subset_of_words,
                                     first,
                                     last,
                                     answers,
                                     total_item_count
                                  )
                          );
      }
   }

   for (my_uint_t i{0}; i < NUM_THREADS; ++i)
   {
      auto results{futures[i].get()};
      entropies.merge(results);
   }
}

string compare(const string &answer, const string &guess)
{
   assert(answer.size() == WORD_LENGTH);
   assert(guess.size() == WORD_LENGTH);

   string rval{"*****"};

   unordered_map<char, my_uint_t> char_count_in_answer{};
   unordered_map<char, my_uint_t> char_count_in_guess{};
   unordered_map<char, my_uint_t> char_count_marked{};

   // Get count of characters in answer and guess
   for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
   {
      ++char_count_in_answer[answer[i]];
      ++char_count_in_guess[guess[i]];
   }

   // Mark green squares
   for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
   {
      if (guess[i] == answer[i])
      {
         rval[i] = 'g';
         ++char_count_marked[guess[i]];
      }
   }

   // Mark squares that are definitely black
   for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
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
      for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
      {
         if (rval[i] != '*')
            continue;

         char guess_char{guess[i]};
         my_uint_t num_guess_char_marked_so_far{char_count_marked[guess_char]};
         my_uint_t num_guess_char_to_be_marked{char_count_in_answer[guess_char]};

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

void get_user_input(const string &prompt, const regex &re, string &user_input)
{
   while (true)
   {
      smatch m;

      cout << prompt << ": ";
      cin >> user_input;

      if (! regex_match(user_input, m, re))
      {
         cout << endl;
         cout << "Invalid: " << user_input << endl << endl;

         continue;
      }
      else
         break;
   }
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

   cout << endl;
}

namespace
{
   entropy_words_map_t iterate_over_subset_of_words(
                                                      word_list_t::const_iterator first,
                                                      word_list_t::const_iterator last,
                                                      const word_list_t &answers,
                                                      my_uint_t total_item_count
                                                   )
   {
      entropy_words_map_t entropies{};

      for (
             word_list_t::const_iterator iter{first};
             iter != last;
             ++iter
          )
      {
         const string &guess{*iter};

         // bin --> item count in bin
         bin_item_count_map_t bins;

         for (const string &answer : answers)
            ++bins[compare(answer, guess)];

         // bin --> probability of landing in bin
         bin_probability_map_t probabilities;

         for (const auto &[bin, item_count] : bins)
            probabilities[bin] = item_count / (entropy_t) total_item_count;

         entropy_t entropy{};

         for (const auto &[word, prob] : probabilities)
            entropy -= prob * log2(prob);

         entropies.insert({entropy, guess});
      }

      return entropies;
   }
}
