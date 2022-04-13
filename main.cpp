#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

using namespace std;

#include "tools.h"

int main(int argc, char *argv[])
{
   assert(argc == 3);

   cout << compare(argv[1], argv[2]) << endl;

#if 0
   set<string> all_words;
   set<string> allowed_guesses;
   set<string> answers;

   // Load allowed guesses
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

   // Load possible answers
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
#endif
}
