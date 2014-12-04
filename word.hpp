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

#ifndef WORD_H
#define WORD_H

#include <iostream>
#include <list>
#include <map>
#include <vector>

using namespace std;

struct Word {
        Word() : word_(""), weight_(1) {}
        Word(const Word& obj) : word_(obj.word_), weight_(obj.weight_) {
                for (auto& x : obj.chain_) {
                        unique_ptr<Word> temp(new Word(*x.second));
                        chain_.insert(pair<string, unique_ptr<Word> >(
                                x.first, move(temp)));
                }
                for (auto x : obj.characteristics_) {
                        characteristics_.push_back(x);
                }
        }
        Word(const string& txt) :
                word_(txt), weight_(1) {}

        void outputTopSentence() {
                cout << word_ << " ";
                Word* topWord = new Word();
                topWord->weight_ = 0;

                if (chain_.size() > 0) {
                        for (auto& x : chain_) {
                                if (x.second->weight_ > topWord->weight_)
                                        topWord = new Word(*x.second);
                        }
                        topWord->outputTopSentence();
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

#endif // WORD_H
