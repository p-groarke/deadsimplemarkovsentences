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
#pragma once

#include <cstdio>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <utility>
#include <vector>

#include "word.hpp"

using namespace std;

struct Database {

        Database() {}

        friend istream& operator>>(istream& is, Database& db) {

                ifstream ifs;
                ifs.open(db.inputFilename_, ios::in | ios::binary);

                if (!ifs.is_open()) {
                        cout << "Couldn't read " << db.inputFilename_ << endl;
                        return is;
                }

                db.ss << ifs.rdbuf();

                ifs.close();
                return db.ss;
        }

        unique_ptr<map<string, unique_ptr<Word> > > loadFile(int& markovLength, string f = "data.dsmc")
        {
                unique_ptr<map<string, unique_ptr<Word> > > myMap(
                                new map<string, unique_ptr<Word> >);

                ifstream ifs;
                string backupFile = f;
                backupFile.insert(0, ".");
                backupFile += ".bak";

                ifs.open(backupFile, ios::in | ios::binary);
                if (!ifs.good()) {
                        ifs.open(f, ios::in | ios::binary);
                }

                if (!ifs.is_open()) {
                        cout << "Couldn't load " << f << endl;
                        return move(myMap);
                }

                ifs >> markovLength;
                size_t mapSize = 0;
                ifs >> mapSize;
                cout << "Current database size: " << mapSize << endl;

                for (int i = 0; i < mapSize; ++i) {
                        string tempKey;
                        unique_ptr<Word> tempWord(new Word());
                        ifs >> tempKey >> *tempWord;
                        myMap->insert(
                                pair<string, unique_ptr<Word> >(tempKey, move(tempWord)));
                }

                for (auto& x : *myMap)
                        x.second->printInfo();

                ifs.close();
                return move(myMap);
        }

        void save(unique_ptr<map<string, unique_ptr<Word> > >& l, int markovLength, string f = "data.dsmc")
        {
                cout << "Saving database " << f << endl;
                string backupName = f;
                backupName.insert(0, ".");
                backupName += ".bak";

                ifstream  src(backupName, std::ios::binary);
                ofstream  dst(backupName, std::ios::binary);

                ofstream ofs;
                ofs.open(f, ios::out | ios::binary);

                if (!ofs.is_open()) {
                        cout << "Couldn't save " << f << endl;
                        return;
                }

                ofs << markovLength << endl;
                ofs << l->size() << endl;
                for (auto& x : *l) {
                        ofs << x.first << endl << *x.second;
                }

                remove(backupName.c_str());
                ofs.close();
        }

        string inputFilename_;
        stringstream ss;
};
