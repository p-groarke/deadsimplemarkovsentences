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

#ifndef SPEAK_H
#define SPEAK_H

#include <memory>

#include "word.hpp"

void speak(unique_ptr<map<string, unique_ptr<Word> > >& myMap, int numSentences, int markovLength)
{
        vector<unique_ptr<Word> > sortedByWeight;
        for (auto& x : *myMap) {
                unique_ptr<Word> temp(new Word(*x.second));
                sortedByWeight.push_back(move(temp));
        }

        sort(sortedByWeight.begin(), sortedByWeight.end(), [](unique_ptr<Word>& w1,
                unique_ptr<Word>& w2) -> bool {
                return w1->weight_ > w2->weight_;
        });

        int i = 0;
        for (auto& x : sortedByWeight) {
                // Only pick the first word with a capital letter.
                if (!isupper(x->word_[0]))
                        continue;

                vector<string> sentence;
                x->outputTopSentence(sentence);

                while (!isEndOfSentence(sentence.back()))
                {
                        int beforeLast = sentence.size() - (markovLength - 1);
                        auto temp = myMap->find(sentence[beforeLast]);
                        if (temp != myMap->end()) {
                                sentence.push_back(temp->second->getWord(
                                        sentence.back())->topWord()->word_);
                        } else {
                                break;
                        }
                }

                for (auto x : sentence)
                        cout << x << " ";
                cout << "   (" << x->weight_ << ")" << endl;
                ++i;
                if (i == numSentences)
                        break;
        }
}

#endif //SPEAK_H