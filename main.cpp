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

/*
 * This software builds markov chains of length n.
 */

#include "discussion.hpp"
#include "gutenbergparser.hpp"
#include "irc.hpp"
#include "loadandsave.hpp"
#include "reader.hpp"
#include "voice.hpp"
#include "word.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <thread>
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

atomic_bool quitApp(false); // = false; is WRONG

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
        cout << setw(25) << left << "--stdin" << "Learn from input pipe." << endl;
        cout << setw(25) << left << "    -m [number]" << "Markov length (default 3)." << endl;
        cout << setw(25) << left << "    --gutenberg" << "Clean books from Gutenberg Project." << endl;

        //Output
        cout << endl << "* Output:" << endl << endl;
        cout << setw(25) << left << "--speak" << "Speak to general output." << endl;
        cout << setw(25) << left << "    -n [number]" << "Generate n number of sentences (default 1)." << endl;
        cout << setw(25) << left << "--rand [number]" << "Random range. Ex. 0.1, will choose 10% top words)." << endl;

        cout << endl << "* Irc:" << endl << endl;
        cout << setw(25) << left << "--irc" << "Connect to Irc." << endl;
        cout << setw(25) << left << "    --nick " << "Nickname and username (default Melinda87_2)." << endl;
        cout << setw(25) << left << "    --server " << "Server address (default irc.twitch.tv)." << endl;
        cout << setw(25) << left << "    --channel" << "Join a channel (default #socapex)." << endl;
        cout << setw(25) << left << "    --pass" << "Server password." << endl;
        cout << endl << endl;
}

void userCommands()
{
        string userIn;
        while (cin >> userIn) {
                if (userIn.find("quit") != string::npos) {
                        quitApp.store(true);
                        cout << "Shutting down system and saving database." << endl;
                        break;
                }
        }
}

int main (int argc, char ** argv)
{
        // Set randomness
        srand (time(NULL));

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

        Irc ircBot = Irc(
                "melinda87_2",
                "irc.twitch.tv",
                "#socapex",
                "oauth:fhj2izp4nnhbt137fga5gats3bompu");

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
                { "rand", required_argument, 0, 'r'},

                //Irc
                { "irc", no_argument, 0, 'i' },
                { "nick", required_argument, 0, 'N' },
                { "server", required_argument, 0, 'I' },
                { "channel", required_argument, 0, 'c' },
                { "pass", required_argument, 0, 'p' }

        };

        int option_index = 0;

        int opt = 0;
        while ((opt = getopt_long(argc, argv, "hsm:gSn:iN:I:c:p:",
                long_options, &option_index)) != -1) {

                switch (opt) {
                        // Input
                        case 'm': markovLength = atoi(optarg); break;
                        case 's': doRead = true; break;
                        case 'g': doGutenberg = true; break;

                        // Output
                        case 'n': numSentences = atoi(optarg); break;
                        case 'S': doSpeak = true; break;
                        case 'r': voice->setRandom(mainWordList_->size() * atof(optarg)); break;

                        // Irc
                        case 'i': doIrc = true; break;
                        case 'N': ircBot.nick_ = string(optarg); break;
                        case 'I': ircBot.address_ = optarg; break;
                        case 'c': ircBot.channel_ = optarg; break;
                        case 'p': ircBot.pass_ = optarg; break;

                        // Help & error
                        case 'h': printHelp(); break;
                        case '?': cout << "What happened?" << endl; break;
                        default: printHelp();
                }
        }


        //// MAIN ////

        if (doGutenberg && doRead) {
                while (cin >> *gutenbergParser >> *reader) {}
                reader->generateMainTree(mainWordList_, markovLength);
                save(mainWordList_, markovLength);
        }

        if (!doGutenberg && doRead) {
                while (cin >> *reader) {}
                reader->generateMainTree(mainWordList_, markovLength);
                for (auto& x : *mainWordList_)
                        cout << x.first << endl;

                save(mainWordList_, markovLength);
        }

        if (doSpeak && !doIrc) {
                voice->generateSortedVector(mainWordList_);
                for (auto& x : voice->speak(numSentences, 1))
                        cout << x << endl;
        }

        if (doIrc) {
                // Start everything up
                thread ircThread(&Irc::start, &ircBot);
                thread userInputLoop(userCommands);

                while (!quitApp) {
                        if (doSpeak) {
                                ircBot.say(voice->speak(numSentences, 1));
                        }

                        reader->addToHugeAssWordList(ircBot.getCachedSentences());
                        reader->generateMainTree(mainWordList_, markovLength);
                        save(mainWordList_, markovLength);

                        this_thread::sleep_for(chrono::seconds(45));
                }
                // Cleanup
                userInputLoop.join();
                ircBot.stop.store(true);
                ircThread.join();
                save(mainWordList_, markovLength);
        }

        return 0;
}

