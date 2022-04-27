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
   my_uint_t chars_left_to_mark{WORD_LENGTH};
   string rval{"*****"};

   unordered_map<char, my_uint_t> char_count_in_answer{};
   unordered_map<char, my_uint_t> char_count_marked{};

   // Get counts of characters in answer
   for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
      ++char_count_in_answer[answer[i]];

   // Mark green squares
   for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
   {
      if (guess[i] == answer[i])
      {
         rval[i] = 'g';
         ++char_count_marked[guess[i]];
         --chars_left_to_mark;
      }
   }

   // Mark squares that are definitely black
   for (my_uint_t i{0}; i < WORD_LENGTH; ++i)
   {
      if (char_count_in_answer[guess[i]] == 0)
      {
         rval[i] = 'b';
         --chars_left_to_mark;
      }
   }

   // Mark yellow squares and remaining black squares. This can be tricky.
   // Consider this case (with made up words):
   // Answer            : abcda
   // Guess             : azaza
   // Correct response  : gbbbg
   // Incorrect response: gbybg
   while (chars_left_to_mark > 0)
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
            --chars_left_to_mark;
         }
         else
         {
            rval[i] = 'b';
            --chars_left_to_mark;
         }
      }
   }

   return rval;
}

void get_guess(
                 const word_list_t &all_words_unfiltered,
                 const word_list_t &answers_filtered,
                 my_uint_t round,
                 string &guess
              )
{
   static bool initialized {false};

   regex word_regex;

   if (! initialized)
   {
      stringstream word_ss;

      word_ss << "[a-z]{" << WORD_LENGTH << "}";
      word_regex = word_ss.str();

      initialized = true;
   }

   if (answers_filtered.size() == 0)
      throw runtime_error("No possible answer words remain. Something is wrong!");
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

      cout << "Possible answers remaining: " << answers_filtered.size() << endl;

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
      {
         if (allowed_answers_filename == "wordle-answers-alphabetical.txt")
            entropies.insert({0.5, "soare"});
         else if (allowed_answers_filename == "wordmaster-answers-alphabetical.txt")
            entropies.insert({0.5, "tares"});
         else
            throw runtime_error("Unknown dictionary!");
      }

      guess = entropies.cbegin()->second;

      cout << "Possible answers remaining: " << answers_filtered.size() << endl;

      cout << "Best guess by entropy taken over all guess words: "
           << guess
           << " ("
           << entropies.cbegin()->first
           << ")"
           << endl;
   }

   // Let the user manually input the guess if that's what they want.
   // This is useful when solving mutiple puzzles simultaneously.
   //
   // This must be done after the code above so that the user knows
   // what the "best" word is so they can use it if they want to.
   //
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

void save_word_list(const word_list_t &word_list, const string &filename)
{
   ofstream word_list_file{filename};

   for (const string &word : word_list)
      word_list_file << word << endl;

   word_list_file.close();
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
