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
#ifndef READSTDIN_H
#define READSTDIN_H

#include <iostream>
#include <list>
#include <vector>

#include "loadandsave.hpp"
#include "word.hpp"

using namespace std;

bool isEndOfSentence(unique_ptr<Word>& w)
{
        if (w->word_.find(".") != string::npos ||
            w->word_.find("!") != string::npos ||
            w->word_.find("?") != string::npos)
                return true;
        return false;
}

void readSTDIN(unique_ptr<map<string, unique_ptr<Word> > >& myMap, int markovLength = 3)
{
        cout << "Enter text (use ctrl+D to stop)." << endl;

        string userText;
        list<unique_ptr<Word> > hugeAssWordList_;

        // Get input.
        while (cin >> userText) {
                hugeAssWordList_.push_back(unique_ptr<Word>(new Word(userText)));
        }

        int currentRead = 0;
        for (auto x = hugeAssWordList_.begin(); x != hugeAssWordList_.end();) {
        //for (auto& x : hugeAssWordList_) {
                list<unique_ptr<Word> > temp;

                if (currentRead + markovLength > hugeAssWordList_.size())
                        break;

                // Create a temporary list of n words (n == markov chain length)
                auto y = x;
                for (int i = 0; i < markovLength; ++i) {
                        if (i == 0) {
                                temp.push_back(move(*x));
                                ++y;
                                ++x;
                        } else {
                                temp.push_back(unique_ptr<Word>(new Word(**y)));
                                ++y;
                        }
                }

                unique_ptr<Word> wt = move(temp.front());
                temp.pop_front();

                auto ret = myMap->insert(
                        pair<string, unique_ptr<Word> >(wt->word_, move(wt)));

                if (ret.second == false) {
                        ret.first->second->weight_++;
                }

                ret.first->second->addWordInChain(temp);

                ++currentRead;

        }
        // Debug output of root tree.
        for (auto& x : *myMap)
                x.second->printInfo();

        // Save the list to the database.
        save(myMap);
}

#endif //READSTDIN_H
