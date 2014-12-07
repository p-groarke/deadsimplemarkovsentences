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

#include "gutenbergparser.hpp"
#include "irc.h"
#include "loadandsave.hpp"
#include "reader.hpp"
#include "voice.hpp"
#include "word.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <thread>
#include <unistd.h>
#include <vector>

#ifdef __APPLE__ // TODO: Add linux
#include <getopt.h>
#else
#include "getopt.h"
#endif

using namespace std;



//// CONSTs i& globals ////

int markovLength = 3;
int numSentences = 1;
bool doRead, doGutenberg, doSpeak, doIrc = false;

void printHelp()
{
        cout << endl << "#########################" << endl
        << "Dead Simple Markov Chains" << endl
        << "v0.01" << endl
        << "Philippe Groarke <philippe.groarke@gmail.com>" << endl
        << "#########################" << endl
        << endl;

        cout << "Usage: dsmc [options]" << endl << endl;

        cout << "========" << endl
        << "Options" << endl
        << "========" << endl
        << endl;

        //Input
        cout << "* Input:" << endl << endl;
        cout << setw(10) << left << "--stdin" << "Learn from input pipe." << endl;
        cout << setw(10) << left << "    -m [number]" << "Markov length (default 3)." << endl;
        cout << setw(10) << left << "    --gutenberg" << "Clean books from Gutenberg Project." << endl;

        //Output
        cout << endl << "* Output:" << endl << endl;
        cout << setw(10) << left << "--speak" << "Speak to general output." << endl;
        cout << setw(10) << left << "    -n [number]" << "Generate n number of sentences (default 1)." << endl;

        cout << setw(10) << left << "--irc" << "Connect to Irc." << endl;
        cout << endl << endl;
}

int main (int argc, char ** argv)
{
        // The map is the first word, the attached map is ordered by which word
        // is used most often after it.
        unique_ptr<map<string, unique_ptr<Word> > > mainWordList_(
                        new map<string, unique_ptr<Word> >);
        mainWordList_ = loadFile(markovLength);

        unique_ptr<Reader> reader(new Reader());
        unique_ptr<GutenbergParser> gutenbergParser(new GutenbergParser());

        unique_ptr<Voice> voice(new Voice());
        voice->setMarkov(markovLength);
        voice->generateSortedVector(mainWordList_);



        //// MENU ////

        if (argc == 1) {
                printHelp();
                return 0;
        }

        static struct option long_options[] =
        {
                { "help", no_argument, 0, 'h' },

                //Input
                { "stdin", no_argument, 0, 's' },
                { "m", required_argument, 0, 'm' },
                { "gutenberg", no_argument, 0, 'g' },

                //Output
                { "speak", no_argument, 0, 'S' },
                { "n", required_argument, 0, 'n' },

                //Irc
                { "irc", no_argument, 0, 'i' }

        };

        int option_index = 0;

        int opt = 0;
        while ((opt = getopt_long(argc, argv, "hsm:gSn:i",
                long_options, &option_index)) != -1) {

                switch (opt) {
                        // Input
                        case 'm': markovLength = atoi(optarg); break;
                        case 's': doRead = true; break;
                        case 'g': doGutenberg = true; break;

                        // Output
                        case 'n': numSentences = atoi(optarg); break;
                        case 'S': doSpeak = true; break;

                        // Irc
                        case 'i': doIrc = true; break;

                        // Help & error
                        case 'h': printHelp(); break;
                        case '?': cout << "What happened?" << endl; break;
                        default: printHelp();
                }
        }



        //// MAIN ////

        if (doGutenberg) {
                cout << "Testing gutenberg parser." << endl << endl;

                while (cin >> *gutenbergParser >> *reader) {}
                reader->generateMainTree(mainWordList_, markovLength);
                save(mainWordList_, markovLength);
        }

        if (!doGutenberg && doRead) {
                while (cin >> *reader) {}
                reader->generateMainTree(mainWordList_, markovLength);
                save(mainWordList_, markovLength);
        }

        if (doSpeak) {
                voice->generateSortedVector(mainWordList_);
                voice->speak(numSentences);
        }

        if (doIrc) {
                Irc bot = Irc(
                        "NICK melinda87_2\r\n",
                        "USER melinda87_2 hostname servername :Bob Affet\r\n",
                        "irc.twitch.tv",
                        "JOIN #socapex\r\n",
                        "PASS oauth:p74yztnqkje36y5a0fe2v7herwh5v7\r\n");

                        thread ircThread(&Irc::start, &bot);
                        //bot.start();
                        while (true) {
                                sleep(120);
                                bot.say(voice->speakTwitch());
                        }
        }

        return 0;
}

