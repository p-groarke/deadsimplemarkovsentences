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

#ifndef VOICE_H
#define VOICE_H

#include <chrono>
#include <memory>
#include <random>

#include "word.hpp"

struct Voice {

        Voice() {
                unsigned seed = chrono::system_clock::now().time_since_epoch().count();
                mersenne_gen = mt19937(seed);
        }

        void setRandom(int range, float rangePercent) {
                randomGen = uniform_int_distribution<int>(0, range);
                randomPercent = rangePercent;
        }

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

        // Will output the position
        vector<unique_ptr<Word> > findFirstWords() // Higher range is more random
        {
                static int lastRandomNumber = 0; // Don't repeat sentences.
                int randomPos = -1;

                if (randomGen.b() != 0) {
                        do {
                                randomPos = randomGen(mersenne_gen);
                        } while (randomPos == lastRandomNumber);
                } else {
                        randomPos = randomGen(mersenne_gen);
                }
                lastRandomNumber = randomPos;

                vector<unique_ptr<Word> > sentence;

                for (int i = randomPos; i < sortedVector.size(); ++i) {
                        // Loop around if at end
                        if (i + 1 >= sortedVector.size())
                                i = 0;

                        // First word needs to start a sentence
                        if (sortedVector[i]->characteristics_.find(CHARACTER_BEGIN)
                        == sortedVector[i]->characteristics_.end())
                                continue;

                        sortedVector[i]->outputTopSentence(sentence, randomPercent);
                        break;
                }
                return sentence;
        }

        vector<string> speak(int numSentences = 1,
                int minWords = 3, int maxWords = 20)
        {
                vector<string> ret;

                for (int i = 0; i < numSentences; ++i) {

                        string outputSentence;
                        // Get a sentence beginning in 75% most used.
                        vector<unique_ptr<Word> > sentence =
                                findFirstWords();

                        if (sentence.size() < 1)
                            return ret;

                        while (sentence.back()->characteristics_.find(CHARACTER_ENDL)
                        == sentence.back()->characteristics_.end()) {
                                // Get the word before the last one, so we will get a
                                // chain of 2 for example.
                                int beforeLast = sentence.size() - (markovLength_ - 1);

                                string s = sentence[beforeLast]->word_;
                                auto temp = find_if(sortedVector.begin(), sortedVector.end(),
                                        [s](unique_ptr<Word>& w1) -> bool {
                                                return w1->word_ == s;
                                });

                                // Now, get a top word AFTER the last word in sentence.
                                // Ex: beforeLast->currentWord->newTopWord is what we
                                // are doing.
                                if (temp != sortedVector.end()) {
                                        sentence.push_back((*temp)->getWord(
                                                sentence.back())->topWord());
                                } else { // Just make sure we are not at the complete end.
                                        break;
                                }
                        }

                        for (auto& x : sentence) {
                                outputSentence += x->word_ + " ";
                        }


                        // For twitch, make sentences smaller. Number of words.
                        if (sentence.size() > maxWords || sentence.size() < minWords) {
                                outputSentence = speak(1, minWords, maxWords).back();
                        }
                        ret.push_back(outputSentence);
                }

                return ret;
        }

        vector<unique_ptr<Word> > sortedVector;
        mt19937 mersenne_gen;
        uniform_int_distribution<int> randomGen;
        int markovLength_ = 3;
        float randomPercent = 0.0;
};
#endif //VOICE_H
