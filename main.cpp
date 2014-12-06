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
#include "speak.hpp"
#include "word.hpp"

using namespace std;



//// CONSTs i& globals ////

int markovLength = 3;
int numSentences = 10;


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
            << endl
            << "--load     Load saved database." << endl
            << endl
            << "--speak    Bot will speak sentences." << endl
            << "    --num [n]    n = number of sentences to generate (default 10)." << endl
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
                        if (argc <= i+1) {
                                // nothing
                        } else if ((string)argv[++i] == "--markov") {
                                markovLength = atoi(argv[++i]);
                                cout << "Markov length is: " << markovLength << endl;
                        }
                        readSTDIN(mainWordList_, markovLength);
                        return 0;
                }
                if ((string)argv[i] == "--load") {
                        mainWordList_ = loadFile(markovLength);
                }
                if ((string)argv[i] == "--speak") {
                        if (argc <= i+1) {
                                // nothing
                        } else if ((string)argv[++i] == "--num") {
                                numSentences = atoi(argv[++i]);
                                cout << "Generating " << numSentences << " top sentences." << endl;
                        }
                        speak(mainWordList_, numSentences, markovLength);
                }
        }
        return 0;
}

