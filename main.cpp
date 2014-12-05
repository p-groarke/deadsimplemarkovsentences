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

/*
 * This software builds markov chains of length n.
 */

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "loadandsave.hpp"
#include "readstdin.hpp"
#include "word.hpp"

using namespace std;



//// CONSTs i& globals ////

int markovLength = 3;



//// FUNCTORS ////



struct orderByWeight {
        bool operator()(pair<string, unique_ptr<Word> >& w1,
                        pair<string, unique_ptr<Word>>& w2) const;
};

bool markovWeightOrdering (unique_ptr<Word>& w1, const unique_ptr<Word>& w2);
bool alphabetOrder (const unique_ptr<Word>& w1, const unique_ptr<Word>& w2);

bool orderByWeight::operator()(pair<string, unique_ptr<Word> >& w1,
                        pair<string, unique_ptr<Word>>& w2) const
{
        return w1.second->weight_ > w2.second->weight_;
}

bool listOrderByWeight(unique_ptr<Word>& w1, unique_ptr<Word>& w2)
{
        return w1->weight_ > w2->weight_;
}

// w2 will be removed if true
bool markovWeightOrdering (unique_ptr<Word>& w1, const unique_ptr<Word>& w2) {
        if (w1->word_ == w2->word_) {
                w1->weight_++;
                return true;
        }
        return false;
}

bool alphabetOrder (const unique_ptr<Word>& w1, const unique_ptr<Word>& w2) {
        unsigned int i=0;
        while ((i < w1->word_.length()) && (i < w2->word_.length())) {
                if (tolower(w1->word_[i]) < tolower(w2->word_[i])) return true;
                else if (tolower(w1->word_[i]) > tolower(w2->word_[i])) return false;
                ++i;
        }
        return (w1->word_.length() < w2->word_.length());
}



//// MAIN ////



void outputHelp()
{
        cout << endl
             << "#####################################" << endl
             << "Dead simple Markov Sentence Framework" << endl
             << "#####################################" << endl << endl
             << "Philippe Groarke <philippe.groarke@gmail.com>" << endl << endl
             << "--save     Input text using stdin." << endl
             << "    --markov [n]    n = length of chains (default 3)." << endl
             << "--load     Load saved database." << endl
             << endl;
}

int main (int argc, char ** argv)
{
        // The map is the first word, the attached map is ordered by which word
        // is used most often after it.
        unique_ptr<map<string, unique_ptr<Word> > > mainWordList_(
                        new map<string, unique_ptr<Word> >);

        for (int i = 0; i < argc; ++i) {
                if ((string)argv[i] == "--help") {
                        outputHelp();
                        return 0;
                }
                if ((string)argv[i] == "--save") {
                        if ((string)argv[++i] == "--markov") {
                                markovLength = atoi(argv[++i]);
                                cout << "Markov length is: " << markovLength << endl;
                        }
                        readSTDIN(mainWordList_, markovLength);
                        return 0;
                }
                if ((string)argv[i] == "--load") {
                        mainWordList_ = loadFile();
                }
                if ((string)argv[i] == "--markov") {
                        markovLength = atoi(argv[++i]);
                        cout << "Markov length is: " << markovLength << endl;
                }
        }

        list<unique_ptr<Word> > sortedByWeight;
        for (auto& x : *mainWordList_) {
                unique_ptr<Word> temp(new Word(*x.second));
                sortedByWeight.push_back(move(temp));
        }

        sortedByWeight.sort(listOrderByWeight);

        cout << "Top sentences" << endl;
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
                        auto temp = mainWordList_->find(sentence[beforeLast]);
                        if (temp != mainWordList_->end()) {
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
                if (i == 10)
                        break;
        }

        return 0;
}

