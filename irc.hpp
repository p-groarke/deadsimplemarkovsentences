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

#ifndef IRC_H_
#define IRC_H_

#include "word.hpp"

#include <atomic>
#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>

using namespace std;

const int MAXDATASIZE = 1000;

class Irc {
public:
        Irc(const string& nick, const string& address,
                const vector<string>& channels, const string& talkChannel,
                const string& pass = "", const string& port = "6667");
        Irc(const Irc& obj);
        virtual ~Irc();

        void start();
        void say(const string& msg);
        void say(const vector<string>& msg);
        unique_ptr<vector<unique_ptr<Word> > > getCachedSentences();

        atomic<int> socket_; //socket descriptor
        atomic_bool stop = {false};
        mutex sentences_mutex;
        mutex users_mutex;
        string nick_;
        string address_;
        string talkChannel_;
        vector<string> channels_;
        bool allChans_;
        string pass_;
        string port_;

private:
        bool ircConnect();
        void sendConnect();
        bool isConnected(const string& msg);
        bool sendData(const string& msg);
        vector<string> recvData();
        void parseOutput(const string& msg);
        string formatNiceOutput(const string& msg);
        string formatString(const string& command, const string& str);
        string formatPrivMsg(const string& command, const string& channel,
                const string& str);
        void sendPong(const string& msg);
        void doUsersCharacteristics(unique_ptr<vector<unique_ptr<Word> > >& vec);
        void doUsersPart();
        void joinAllChannels();

        vector<string> users_;
        vector<string> sentencesToBeParsed; // channel, sentence
        stringstream ss;
};



//// IMPLEMENTATION ////

Irc::Irc(const string& nick, const string& address,
        const vector<string>& channels, const string& talkChannel,
        const string& pass, const string& port) :
        nick_(nick),
        address_(address),
        channels_(channels.begin(), channels.end()),
        talkChannel_(talkChannel),
        pass_(pass),
        port_(port)
{}

Irc::Irc(const Irc& obj) :
        nick_(obj.nick_),
        address_(obj.address_),
        channels_(obj.channels_),
        talkChannel_(obj.talkChannel_),
        pass_(obj.pass_),
        port_(obj.port_)
{}

Irc::~Irc()
{
        close (socket_);
}

void Irc::start()
{
        if (!ircConnect())
                return;

        if (allChans_)
                joinAllChannels();

        while (!stop) {
                //this_thread::sleep_for(chrono::seconds(1));
                for (auto& x : recvData())
                        parseOutput(x);
        }
        return;
}

void Irc::say(const string& msg)
{
        sendData(formatPrivMsg("PRIVMSG", talkChannel_, msg));
}

void Irc::say(const vector<string>& msg)
{
        string out;
        for (const auto& x : msg)
            out += x + " ";

        sendData(formatPrivMsg("PRIVMSG", talkChannel_, out));

}

unique_ptr<vector<unique_ptr<Word> > > Irc::getCachedSentences()
{
        unique_ptr<vector<unique_ptr<Word> > > ret(new vector<unique_ptr<Word> >);

        lock_guard<mutex> lk(sentences_mutex);

        // Lets not segfault this time
        if (sentencesToBeParsed.size() <= 0) {
                return ret;
        }

        for (auto& x : sentencesToBeParsed) {
                stringstream ss(x);
                string word;

                vector<unique_ptr<Word> > tempSentence;
                while(ss >> word) {
                        tempSentence.push_back(unique_ptr<Word>(new Word(word)));
                }

                if (tempSentence.size() <= 0)
                        continue;

                // Since IRC doesnt necessarily have caps or . , add characters here.
                tempSentence.front()->characteristics_.insert(CHARACTER_BEGIN);
                tempSentence.back()->characteristics_.insert(CHARACTER_ENDL);

                ret->insert(ret->end(), make_move_iterator(tempSentence.begin()),
                        make_move_iterator(tempSentence.end()));
        }

        // Find names! And groove tonight, share the spice of life!
        doUsersCharacteristics(ret);
        doUsersPart();

        // Delete the cache of sentences.
        sentencesToBeParsed.erase(sentencesToBeParsed.begin(),
                sentencesToBeParsed.end());

        return ret;
}




//// PRIVATE ////

void Irc::parseOutput(const string& msg)
{
        // Check ping: http://www.irchelp.org/irchelp/rfc/chapter4.html
        if (msg.find("PING") != string::npos) {
                cout << "Found ping: " << msg << endl;
                sendPong(msg);
        } else if (msg == "DISCONNECT") {
                stop.store(true);
                return;
        } else if (msg.find("JOIN") != string::npos
                || msg.find("PART") != string::npos) {

                lock_guard<mutex> lk(users_mutex);

                string x = msg;
                x.erase(0, 1); // Remove :
                if (x.find("!") == string::npos) // Something went wrong, exit.
                        return;

                x.erase(x.find("!")); // Extract name
                if (msg.find("JOIN") != string::npos)
                        x += " JOIN";
                else
                        x += " PART"; // Used later for removing users

                cout << "Found user: " << msg << " ==> " << x << endl;

                users_.push_back(x);
        } else if (msg.find("PRIVMSG") != string::npos
                && msg.find("#") != string::npos) {

                lock_guard<mutex> lk(sentences_mutex);
                cout << "Found sentence: " << msg << " ==> " << formatNiceOutput(msg) << endl;
                sentencesToBeParsed.push_back(formatNiceOutput(msg));

        // } else if (allChans_ && msg.find("353") != string::npos) {

        //         string temp = msg;
        //         if (temp.find(" = ") != string::npos) {
        //                 temp = temp.substr(temp.find(" = "));
        //                 temp.erase(0, 3);
        //                 string users = temp.substr(temp.find(" :"));
        //                 temp = temp.erase(temp.find(" :"));

        //                 stringstream ss(users);
        //                 string singleUser;
        //                 int numU = 0;
        //                 while(ss >> singleUser) {
        //                         ++numU;
        //                 }

        //                 if (numU > 10) {
        //                         cout << "Found " << numU << " users in " << temp << ": " << users << endl;
        //                         channels_.push_back(temp);
        //                 }

        //         } else {
        //                 cout << "Found problem in channel: " << temp << endl;
        //         }


        // } else if (msg.find("/NAMES") != string::npos) {

        //         string tempChan;
        //         int i = 0;
        //         for (auto x : channels_) {
        //                 if (x.size() <= 0)
        //                         continue;
        //                 ++i;
        //                 tempChan += x + ",";
        //         }
        //         tempChan.pop_back();
        //         cout << "Sending: " << formatString("JOIN", tempChan) << endl;
        //         sendData(formatString("JOIN", tempChan));
        } else {
                if (msg.size() > 0)
                        cout << "Found other: " << msg << endl;
        }
}

bool Irc::ircConnect()
{
        bool worked = true;
        struct addrinfo hints, *servinfo;
        memset(&hints, 0, sizeof hints);

        hints.ai_family = AF_UNSPEC; // either IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM; // TCP

        int res;
        cout << address_ << endl;
        if ((res = getaddrinfo(address_.c_str(), port_.c_str(), &hints, &servinfo))
            != 0) {
                fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(res));
                worked = false;
        }

        if ((socket_ = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol)) == -1) {
                perror("client: socket");
                worked = false;
        }

        if (connect(socket_, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
                close (socket_);
                perror("Client Connect");
                worked = false;
        }

        freeaddrinfo(servinfo);

        sendConnect();

        if (fcntl(socket_, F_SETFL, fcntl(socket_, F_GETFL, 0) | O_NONBLOCK)) {
                close (socket_);
                perror("Could not set non-blocking socket");
                worked = false;
        }

        return worked;
}

void Irc::sendConnect()
{
        // Twitch doesn't follow the IRC spec, of course
        if (address_.find("twitch") != string::npos) {
                // Send initial data
                if (pass_.empty())
                    cout << "Twitch requires an oath password." << endl;

                sendData(formatString("PASS", pass_));
                sendData(formatString("NICK", nick_));
                sendData(formatString("USER", nick_));

        } else { // 3 recieves, then send info.
                for (int i = 0; i < 3; ++i) {
                        for (const auto& x : recvData())
                                cout << x << endl;
                }

                if (!pass_.empty())
                        sendData(formatString("PASS", pass_));
                sendData(formatString("NICK", nick_));
                sendData(formatString("USER", nick_));
        }

        // Recieve a response
        for (const auto& x : recvData())
                cout << x << endl;

        // Join channels
        string channelString;
        int i = 0;
        for (auto x : channels_) {
                ++i;
                if (x[0] != '#')
                        x.insert(0, "#");

                if (i != channels_.size())
                        x += ",";

                channelString += x;
        }

        if (!allChans_)
                sendData(formatString("JOIN", channelString));
}

// If we find /MOTD then its ok join a channel
bool Irc::isConnected(const string& msg)
{
        if (msg.find("/MOTD") != string::npos)
                return true;

        return false;
}

bool Irc::sendData(const string& msg)
{
        if (send(socket_, msg.c_str(), msg.size(), 0) == 0) {
                cout << "Error sending data: " << msg << endl;
                return false;
        }

        return true;
}

vector<string> Irc::recvData()
{
        static string lastPiece = "";
        static int lastSize = 0;

        vector<string> ret;
        int numRecv;

        char buf[MAXDATASIZE] = {0};
        numRecv = recv(socket_, buf, MAXDATASIZE - 1, 0);
        //buf[numRecv]='\0';

        if (numRecv == 0) { // Connection closed
                cout << "---- No packets received, closing connection. ----" << endl;
                ret.push_back("DISCONNECT");
                return ret;
        }

        // if (numRecv == -1) { // No data
        //         //cout << "---- Didn't recieve data. ----" << endl;
        //         //ret.push_back("");
        //         return ret;
        // }

        bool curruptedData = false;
        string temp(buf);

        if (temp.size() <= 0)
                return ret;

        if (temp.back() != '\n')
                curruptedData = true;

        stringstream ss(temp);

        if (lastPiece != "") { // Reconstruct broken sentence
                getline(ss,temp,'\n');
                temp.erase(std::remove(temp.begin(), temp.end(), '\r'), temp.end());
                ret.push_back(lastPiece + temp);
                lastPiece = "";

                cout << "RECONSTRUCT: " << ret.back() << endl;
        }

        // Parse the rest
        while(getline(ss, temp, '\n')) { // Chop sentences
                // SURPRISE!!! THERE ARE STILL FUCKING CRs LEFT
                temp.erase(std::remove(temp.begin(), temp.end(), '\r'), temp.end());
                ret.push_back(temp);
        }


        if (numRecv == MAXDATASIZE - 1  || curruptedData) {
                lastPiece = ret.back();
                ret.pop_back();
        }

        lastSize = numRecv;
        return ret;
}

string Irc::formatNiceOutput(const string& msg)
{
        string channel, sentence;

        sentence = msg.substr(msg.find("PRIVMSG"));
        channel = sentence.substr(sentence.find(":") - 1); // channel
        sentence = sentence.substr(sentence.find(":"));
        sentence = sentence.erase(0, 1);
        return sentence;
}


string Irc::formatString(const string& command, const string& str)
{
        string ret = command;

        if (ret[ret.size() - 1] != ' ')
                ret += " ";

        if (ret == "USER ") {
                for (int i = 0; i < 4; ++i) {
                        if (i == 3) {
                                ret += ":";
                                break;
                        } else
                                ret += str + " ";
                }
        }

        if ((ret == "JOIN " || ret == "PRIVMSG ")
            && str.find("#") == string::npos)
                ret += "#";

        ret += str + "\r\n";
        cout << ret;
        return ret;
}

string Irc::formatPrivMsg(const string& command, const string& channel,
                const string& str)
{
        string ret = formatString("PRIVMSG", talkChannel_);
        ret = ret.substr(0, ret.size() - 2);
        ret += " :" + str + "\r\n";
        cout << ret;
        return ret;
}

void Irc::sendPong(const string& msg)
{
        string response = "PONG " + msg.substr(5);

        if (sendData(response)) {
                cout << msg << endl;
                cout << response << endl;
        } else {
                cout << endl << "ERROR: Couldn't send PONG!" << endl;
        }
}

void Irc::doUsersCharacteristics(unique_ptr<vector<unique_ptr<Word> > >& vec)
{
        lock_guard<mutex> lk(users_mutex);
        // Each word
        for (auto& word : *vec) {
                // Find if word is a username, needs lowercase.
                for (const auto& name : users_) {
                        string tempName = name;
                        string tempWord = word->word_;

                        if (tempWord[0] == '@') // remove @
                                tempWord.erase(0, 1);
                        if (tempWord.find(":") != string::npos) // remove username:
                                tempWord.erase(tempWord.find(":"), 1);

                        if (tempName.size() > 6)
                                tempName.erase(tempName.size() - 5); // We know the name ends with JOIN

                        if (tempWord.size() < 5) // dont check small words. Most names > 5
                                break;

                        transform(tempWord.begin(), tempWord.end(), tempWord.begin(), ::tolower);

                        if (tempWord.find(tempName) != string::npos) {
                                cout << endl << "Found name match! " << tempWord << " = " << tempName << endl;
                                word->characteristics_.insert(CHARACTER_NAME);
                                break;
                        }
                }
        }
}

void Irc::doUsersPart()
{
        lock_guard<mutex> lk(users_mutex);

        for (int i = 0; i < users_.size(); ++i) {
                if (users_[i].find("PART") != string::npos) {
                        string username = users_[i];
                        username.erase(username.size() - 5);

                        users_[i] = users_.back();
                        users_.pop_back();

                        users_.erase(remove_if(users_.begin(), users_.end(),
                                [username] (const string& s) {
                                        if (s.find(username) != string::npos)
                                                return true;

                                        return false;
                            }), users_.end());
                }
        }
}

void Irc::joinAllChannels()
{
        sendData("LIST\r\n");
}



#endif /* Irc_H_ */
