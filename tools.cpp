#include <cassert>
#include <cstddef>
#include <map>

using namespace std;

#include "tools.h"

string compare(const string &answer, const string &guess)
{
   assert(answer.size() == 5);
   assert(guess.size() == 5);

   string rval{"*****"};

   map<char, size_t> char_count_in_answer{};
   map<char, size_t> char_count_in_guess{};
   map<char, size_t> char_count_marked{};

   // Get count of characters in answer and guess
   for (size_t i{0}; i < 5; ++i)
   {
      ++char_count_in_answer[answer[i]];
      ++char_count_in_guess[guess[i]];
   }

   // Mark green squares
   for (size_t i{0}; i < 5; ++i)
   {
      if (guess[i] == answer[i])
      {
         rval[i] = 'g';
         ++char_count_marked[guess[i]];
      }
   }

   // Mark squares that are definitely black
   for (size_t i{0}; i < 5; ++i)
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
      for (size_t i{0}; i < 5; ++i)
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
