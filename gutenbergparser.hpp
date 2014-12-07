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

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

enum States {FIRSTNOTICE, BADLINE, END};

struct GutenbergParser {
        GutenbergParser() {}

        bool isAllUpper(const string& s)
        {
                int up = 0;
                for (const auto& x : s) {
                        if (!isalpha(x)) {}
                        else if (isupper(x)) {
                                up++;
                        }
                }
                if ((float(up) / s.size()) > 0.5f)
                        return true;

                return false;
        }

        bool fiveSpaces(const string& s)
        {
                if (s.size() < 6)
                        return false;

                bool spaces = true;
                for (int i = 0; i < 6; ++i) {
                        if (s[i] != ' ')
                                spaces = false;
                }
                return spaces;
        }

        bool fiveSpacesAndNum(const string& s)
        {
                bool spaces = false;
                for (int i = 0; i < s.size(); ++i) {
                        if (s[i] == ' ' && s[i+1] == ' ' &&
                        s[i+2] == ' ' && s[i+3] == ' ' &&
                        s[i+4] == ' ') {
                                if (isdigit(s[i+5])) {
                                        spaces = true;
                                        break;
                                }
                        }
                }
                return spaces;
        }

        friend istream& operator>>(istream& is, GutenbergParser& gp) {
                string userText;

                while(getline(is, userText)) {
                        // Letiral text
                        if (userText.find("The Project Gutenberg EBook of") != string::npos)
                                gp.currentState_ = FIRSTNOTICE;

                        else if (userText.find("Produced by") != string::npos)
                                gp.currentState_ = BADLINE;

                        else if (userText.find("Proofreading Team") != string::npos)
                                gp.currentState_ = BADLINE;

                        else if (userText.find("produced from images generously made available by") != string::npos)
                                gp.currentState_ = BADLINE;

                        else if (userText.find("Internet Archive") != string::npos)
                                gp.currentState_ = BADLINE;

                        else if (userText.find("THE END") != string::npos)
                                gp.currentState_ = END;

                        else if (userText.find("*** END OF THIS PROJECT GUTENBERG") != string::npos)
                                gp.currentState_ = END;

                        else if (userText.substr(0, 5) == "Page ")
                                gp.currentState_ = BADLINE;

                        else if (userText.substr(0, 3) == "***")
                                gp.currentState_ = BADLINE;

                        // Patterns
                        else if (gp.isAllUpper(userText))
                                gp.currentState_ = BADLINE;

                        else if (gp.fiveSpaces(userText))
                                gp.currentState_ = BADLINE;

                        else if (gp.fiveSpacesAndNum(userText))
                                gp.currentState_ = BADLINE;

                        else if (userText.empty() || userText == "\t"
                                || userText == "\r\n" || userText == "\r"
                                || userText == "\n")
                                gp.currentState_ = BADLINE;

                        else
                                gp.currentState_ = 4200;

                        switch(gp.currentState_) {
                                case FIRSTNOTICE:
                                        while(getline(is, userText)) {
                                                if (userText.find("*** START OF THIS PROJECT GUTENBERG EBOOK") != string::npos) {
                                                        gp.currentState_ = 4200; // Default
                                                        break;
                                                }
                                        }
                                break;
                                case BADLINE:

                                break;
                                case END:
                                        while(getline(is, userText)) {}
                                break;
                                default:
                                        userText.erase(remove(userText.begin(), userText.end(), '|'), userText.end());
                                        userText.erase(remove(userText.begin(), userText.end(), '_'), userText.end());
                                        gp.ss << userText << " ";
                                break;
                        }
                }

                return gp.ss;
        }

        friend ostream& operator<<(ostream& os, const GutenbergParser& gp) {
                return os;
        }

        int currentState_ = 4200;
        stringstream ss;
};
