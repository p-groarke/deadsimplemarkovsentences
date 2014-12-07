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
 *
 *  Additional permission under GNU GPL version 3 section 7:
 *
 *  If you modify this program, or any covered work, by linking or
 *  combining it with the OpenSSL project's OpenSSL library (or a
 *  modified version of that library), containing parts covered by the
 *  terms of the OpenSSL or SSLeay licenses, Savoir-Faire Linux Inc.
 *  grants you additional permission to convey the resulting work.
 *  Corresponding Source for a non-source form of such a combination
 *  shall include the source code for the parts of OpenSSL used as well
 *  as that of the covered work.
 */

#ifndef VOICE_H
#define VOICE_H

#include <memory>

#include "word.hpp"

struct Voice {

        Voice() {}

        void setMarkov(int x) {
                markovLength_ = x;
        }

        void generateSortedVector(unique_ptr<map<string, unique_ptr<Word> > >& myMap)
        {
                if (sortedVector.size() > 0)
                        sortedVector.erase(sortedVector.begin(), sortedVector.end());

                for (auto& x : *myMap) {
                        unique_ptr<Word> temp(new Word(*x.second));
                        sortedVector.push_back(move(temp));
                }

                sort(sortedVector.begin(), sortedVector.end(), [](unique_ptr<Word>& w1,
                        unique_ptr<Word>& w2) -> bool {
                        return w1->weight_ > w2->weight_;
                });
        }

        vector<string> findFirstWords(int range) // Higher range is more random
        {
                srand (time(NULL));
                int randomPos = rand() % range;
                vector<string> sentence;

                for (int i = randomPos; i < sortedVector.size(); ++i) {
                        if(!isupper(sortedVector[i]->word_[0])) // Capitalize first letter
                                continue;

                        sortedVector[i]->outputTopSentence(sentence);
                        break;
                }
                return sentence;
        }

        string speakTwitch()
        {
                string outputSentence;
                vector<string> sentence = findFirstWords(sortedVector.size()/2);

                while (!isEndOfSentence(sentence.back())) {
                        int beforeLast = sentence.size() - (markovLength_ - 1);
                        string s = sentence[beforeLast];
                        auto temp = find_if(sortedVector.begin(), sortedVector.end(),
                                [s](unique_ptr<Word>& w1) -> bool {
                                        return w1->word_ == s;
                        });
                        if (temp != sortedVector.end()) {
                                sentence.push_back((*temp)->getWord(
                                        sentence.back())->topWord()->word_);
                        } else {
                                break;
                        }
                }

                for (auto x : sentence) {
                        //cout << x << " ";
                        outputSentence += x + " ";
                }

                // For twitch, make sentences smaller. Number of words.
                if (sentence.size() > 20)
                        outputSentence = speakTwitch();

                return outputSentence;
        }

        void speak(int numSentences)
        {
                int i = 0;
                for (auto& x : sortedVector) {
                        // Only pick the first word with a capital letter.
                        if (!isupper(x->word_[0]))
                                continue;

                        vector<string> sentence;
                        x->outputTopSentence(sentence);

                        while (!isEndOfSentence(sentence.back()))
                        {
                                int beforeLast = sentence.size() - (markovLength_ - 1);
                                string s = sentence[beforeLast];
                                auto temp = find_if(sortedVector.begin(), sortedVector.end(),
                                        [s](unique_ptr<Word>& w1) -> bool {
                                                return w1->word_ == s;
                                });
                                if (temp != sortedVector.end()) {
                                        sentence.push_back((*temp)->getWord(
                                                sentence.back())->topWord()->word_);
                                } else {
                                        break;
                                }
                        }

                        for (auto x : sentence)
                                cout << x << " ";
                        cout << endl;
        //                cout << "   (" << x->weight_ << ")" << endl;
                        ++i;
                        if (i >= numSentences)
                                break;
                }
        }

        vector<unique_ptr<Word> > sortedVector;
        int markovLength_ = 3;
};
#endif //VOICE_H
