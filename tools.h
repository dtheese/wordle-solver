#ifndef TOOLS_INCLUDED
#define TOOLS_INCLUDED

#include <regex>
#include <string>

using namespace std;

#include "type_aliases.h"

void calculate_entropies(
                           const word_list_t &all_words,
                           const word_list_t &answers,
                           entropy_words_map_t &entropies
                        );

string compare(const string &answer, const string &guess);
void get_user_input(const string &prompt, const regex &re, string &user_input);
void load_words(word_list_t &all_words, word_list_t &answers);
void print_entropies(const entropy_words_map_t &entropies);
void save_word_list(const word_list_t &word_list, const string &filename);

#endif
