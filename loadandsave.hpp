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
#ifndef LOADANDSAVE_H
#define LOADANDSAVE_H

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <utility>
#include <vector>

#include "word.hpp"

using namespace std;

unique_ptr<map<string, unique_ptr<Word> > > loadFile(int& markovLength, string f = "data.txt")
{
        unique_ptr<map<string, unique_ptr<Word> > > myMap(
                        new map<string, unique_ptr<Word> >);
        ifstream ifs;
        ifs.open(f, ios::in | ios::binary);

        if (!ifs.is_open())
                return move(myMap);

        ifs >> markovLength;
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

void save(unique_ptr<map<string, unique_ptr<Word> > >& l, int markovLength)
{
        ofstream ofs;
        ofs.open("data.txt", ios::out | ios::binary);

        if (!ofs.is_open())
                return;

        ofs << markovLength << endl;
        ofs << l->size() << endl;
        for (auto& x : *l) {
                ofs << x.first << endl << *x.second;
        }
}

#endif // LOADANDSAVE_H