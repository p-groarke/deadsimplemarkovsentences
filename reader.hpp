/*
 *  Copyright (C) 2014 Philippe Groarke.
 *  Author: Philippe Groarke <philippe.groarke@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 */

#ifndef READSTDIN_H
#define READSTDIN_H

#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "loadandsave.hpp"
#include "word.hpp"

using namespace std;

struct Reader {

        Reader() {}

        bool isEndOfSentence(unique_ptr<Word>& w)
        {
                if (w->word_.find(".") != string::npos ||
                    w->word_.find("!") != string::npos ||
                    w->word_.find("?") != string::npos)
                        return true;
                return false;
        }

        void addCharacteristics(unique_ptr<Word>& w)
        {
                if (isEndOfSentence(w)) {
                        w->characteristics_.insert(CHARACTER_ENDL);
                }

                if (isupper(w->word_[0])) {
                        w->characteristics_.insert(CHARACTER_BEGIN);
                }
        }

        friend istream& operator>>(istream& is, Reader& r) {
                string userText;

                // Get input.
                while (is >> userText) {
                        r.hugeAssWordList_.push_back(unique_ptr<Word>(new Word(userText)));
                        r.addCharacteristics(r.hugeAssWordList_.back());
                }

                return is;
        }

        void addToHugeAssWordList(unique_ptr<Word> w)
        {
                hugeAssWordList_.push_back(move(w));
        }

        void addToHugeAssWordList(unique_ptr<vector<unique_ptr<Word> > > v)
        {
                for (auto& x : *v) {
                        hugeAssWordList_.push_back(move(x));
                }
        }

        bool oneWordSentence(unique_ptr<Word>& w)
        {
                if (w->characteristics_.find(CHARACTER_BEGIN) !=
                        w->characteristics_.end()
                        && w->characteristics_.find(CHARACTER_ENDL) !=
                        w->characteristics_.end())
                        return true;

                return false;
        }

        void generateMainTree(unique_ptr<map<string, unique_ptr<Word> > >& myMap, int markovLength)
        {
                int currentRead = 0;
                for (auto x = hugeAssWordList_.begin(); x != hugeAssWordList_.end();) {
                        list<unique_ptr<Word> > temp;

                        // Still add words.
                        if (currentRead + markovLength > hugeAssWordList_.size()) {
                                temp.push_back(move(*x));
                        } else {
                                // Create a temporary list of n words (n == markov chain length)
                                auto y = x;
                                for (int i = 0; i < markovLength; ++i) {
                                        if (i == 0) {
                                                temp.push_back(move(*x));
                                                // Dont conitnue if it is a 1 sentence sentence.
                                                        if (oneWordSentence(temp.back()))
                                                                break;
                                                ++y;
                                        } else {
                                                temp.push_back(unique_ptr<Word>(new Word(**y)));
                                                ++y;
                                        }
                                }
                        }


                        // Get the first word.
                        unique_ptr<Word> wt = move(temp.front());
                        temp.pop_front();

                        // Copy characterisics or else segfault.
                        unordered_set<string> tempChars = wt->characteristics_;

                        auto ret = myMap->insert(
                                pair<string, unique_ptr<Word> >(wt->word_, move(wt)));

                        // If the word is allready in the main map, add weight
                        // and all characteristics.
                        if (ret.second == false) {
                                ret.first->second->weight_++;
                                for (auto& x : tempChars) {
                                        ret.first->second->characteristics_.insert(x);
                                }
                        }

                        ret.first->second->addWordInChain(temp);

                        ++currentRead;
                        ++x;
                }

                if (hugeAssWordList_.size() > 0)
                        hugeAssWordList_.erase( hugeAssWordList_.begin(),
                                hugeAssWordList_.end());

                // Debug output of root tree.
                //for (auto& x : *myMap)
                //         x.second->printInfo();

        }

        list<unique_ptr<Word> > hugeAssWordList_;
};
#endif //READSTDIN_H
