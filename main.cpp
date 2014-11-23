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
 * This software builds markov chains of length n, the length of sentences.
 */

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

using namespace std;


struct Word;
struct orderByWeight {
        bool operator()(pair<string, unique_ptr<Word> >& w1,
                        pair<string, unique_ptr<Word>>& w2) const;
};

bool markovWeightOrdering (unique_ptr<Word>& w1, const unique_ptr<Word>& w2);
bool alphabetOrder (const unique_ptr<Word>& w1, const unique_ptr<Word>& w2);


//// CLASSES ////


struct Word {
        Word() : word_(""), weight_(1) {}
        Word(const string& txt) :
                word_(txt), weight_(1) {}

        void outputTopSentence() {
                if (chain_.size() > 0) {
                        cout << word_ << " ";
                        int i = 0;
                        for (auto& x : chain_) {
                                x.second->outputTopSentence();
                                ++i;
                                if (i == 10)
                                        break;
                        }
                }
        }

        void addWordInChain(list<unique_ptr<Word> >& wl) {
                if (wl.size() <= 0)
                    return;

                auto ret = chain_.insert(
                        pair<string, unique_ptr<Word> >(
                                wl.front()->word_, move(wl.front())));

                if (ret.second == false) {
                        ret.first->second->weight_++;
                }
                wl.pop_front();
                ret.first->second->addWordInChain(wl);
        }

        void printInfo(int indent = 0) {
                for (int i = 0; i < indent; ++i)
                        cout << " ";

                cout << "\"" << word_ << "\" " << weight_ << endl;

                for (auto& x : chain_) {
                        x.second->printInfo(indent + 1);
                }
        }

        friend ostream& operator<<(ostream& os, const Word& w) {
                os << w.word_ << endl << w.weight_ << endl;
                os << w.characteristics_.size() << endl;
                for (auto& x : w.characteristics_) {
                        os << x << endl;
                }

                os << w.chain_.size() << endl;
                for (auto& x : w.chain_) {
                        os << x.first << endl << *x.second;
                }
                return os;
        }

        friend istream& operator>>(istream& is, Word& w) {
                is >> w.word_ >> w.weight_;
                size_t size;
                is >> size;
                for (int i = 0; i < size; ++i) {
                        string c;
                        is >> c;
                        w.characteristics_.push_back(c);
                }

                is >> size;
                for (int i = 0; i < size; ++i) {
                        string key;
                        unique_ptr<Word> temp(new Word());
                        is >> key >> *temp;
                        w.chain_.insert(
                                pair<string, unique_ptr<Word> >(key, move(temp)));
                }

                return is;
        }

        map<string, unique_ptr<Word> > chain_;
        vector<string> characteristics_;

        string word_;
        int weight_ = 1;

};


//// FUNCTORS ////


bool orderByWeight::operator()(pair<string, unique_ptr<Word> >& w1,
                        pair<string, unique_ptr<Word>>& w2) const
{
        return w1.second->weight_ > w2.second->weight_;
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


//// UTILITIES ////


unique_ptr<map<string, unique_ptr<Word> > > loadFile(string f = "data.txt")
{
        unique_ptr<map<string, unique_ptr<Word> > > myMap(
                        new map<string, unique_ptr<Word> >);
        ifstream ifs;
        ifs.open(f, ios::in | ios::binary);

        if (!ifs.is_open())
                return move(myMap);

        size_t mapSize = 0;
        ifs >> mapSize;
        cout << "Main map size: " << mapSize << endl;

        for (int i = 0; i < mapSize; ++i) {
                string tempKey;
                unique_ptr<Word> tempWord(new Word());
                ifs >> tempKey >> *tempWord;
                myMap->insert(
                        pair<string, unique_ptr<Word> >(tempKey, move(tempWord)));
        }

        //for (auto& x : *myMap)
        //        x.second->printInfo();
        return move(myMap);
}

void save(unique_ptr<map<string, unique_ptr<Word> > >& l)
{
        ofstream ofs;
        ofs.open("data.txt", ios::out | ios::binary);

        if (!ofs.is_open())
                return;

        ofs << l->size() << endl;
        for (auto& x : *l) {
                ofs << x.first << endl << *x.second;
        }
}


void outputHelp()
{
        cout << endl
             << "#####################################" << endl
             << "Dead simple Markov Sentence Framework" << endl
             << "#####################################" << endl << endl
             << "Philippe Groarke <philippe.groarke@gmail.com>" << endl << endl
             << "--save     Input text using stdin." << endl
             << "--load     Load saved database." << endl << endl;
}


bool isEndOfSentence(unique_ptr<Word>& w)
{
        if (w->word_.find(".") != string::npos ||
            w->word_.find("!") != string::npos ||
            w->word_.find("?") != string::npos)
                return true;
        return false;
}


//// MAIN ////


void readSTDIN(unique_ptr<map<string, unique_ptr<Word> > >& myMap)
{
        cout << "Enter text (use ctrl+D to stop)." << endl;

	string userText;
        list<unique_ptr<Word> > hugeAssWordList_;

        // Get input.
	while (cin >> userText) {
                hugeAssWordList_.push_back(unique_ptr<Word>(new Word(userText)));
	}

        // Chop everything up in vectors (sentences) of length n.
        // Then, when a sentence end is detected, construct recursively the
        // chain.
        for (auto x = hugeAssWordList_.begin(); x != hugeAssWordList_.end();) {
                list<unique_ptr<Word> > temp;
                while(!isEndOfSentence(*x)) {
                        cout << (*x)->word_ << endl;
                        temp.push_back(move(*x));
                        ++x;
                        //hugeAssWordList_.erase(x++);
                        if (x == hugeAssWordList_.end())
                                break;
                }
                if (x != hugeAssWordList_.end()) {
                        cout << "End of sentence detected: " << (*x)->word_ << endl;
                        temp.push_back(move(*x));
                        ++x;
                }
                //hugeAssWordList_.erase(x++);

                unique_ptr<Word> wt = move(temp.front());
                temp.pop_front();

                auto ret = myMap->insert(
                        pair<string, unique_ptr<Word> >(wt->word_, move(wt)));

                if (ret.second == false) {
                        ret.first->second->weight_++;
                }

                ret.first->second->addWordInChain(temp);
        }
        // Debug output of root tree.
        for (auto& x : *myMap)
                x.second->printInfo();

        // Save the list to the database.
        save(myMap);
}


unique_ptr<map<string, unique_ptr<Word> > > loadData()
{
        return loadFile();
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
                        readSTDIN(mainWordList_);
                        return 0;
                }
                if ((string)argv[i] == "--load") {
                        mainWordList_ = loadData();
                }
        }

        int i = 0;
        for (auto& x : *mainWordList_) {
                x.second->outputTopSentence();
                cout << endl;
                ++i;
                if (i == 10)
                        break;
        }

        return 0;
}

