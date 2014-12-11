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
                const string& channel, const string& pass = "",
                const string& port = "6667");
        Irc(const Irc& obj);
        virtual ~Irc();

        void start();
        void say(const string& msg);

        unique_ptr<vector<unique_ptr<Word> > > getCachedSentences();

        friend ostream& operator<<(ostream& os, const Irc& irc) {
                return os;
        }

        friend istream& operator>>(istream& is, Irc& irc) {
                irc.ss.str("");
                irc.ss.clear();
                for (auto& x : irc.sentencesToBeParsed) {
                        irc.ss << x << " ";
                }

                irc.sentencesToBeParsed.erase(irc.sentencesToBeParsed.begin(),
                        irc.sentencesToBeParsed.end());
                return irc.ss;
        }

        atomic<int> socket_; //socket descriptor
        atomic_bool stop = {false};
        mutex sentences_mutex;
        mutex users_mutex;
        string nick_;
        string address_;
        string channel_;
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

        vector<string> users_;
        vector<string> sentencesToBeParsed; // channel, sentence
        stringstream ss;
};



//// IMPLEMENTATION ////

Irc::Irc(const string& nick, const string& address,
        const string& channel, const string& pass, const string& port) :
        nick_(nick),
        address_(address),
        channel_(channel),
        pass_(pass),
        port_(port)
{}

Irc::Irc(const Irc& obj) :
        nick_(obj.nick_),
        address_(obj.address_),
        channel_(obj.channel_),
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

        while (!stop) {
                this_thread::sleep_for(chrono::seconds(1));
                for (auto& x : recvData())
                        parseOutput(x);
        }
        return;
}

void Irc::say(string const& msg)
{
        sendData(formatPrivMsg("PRIVMSG", channel_, msg));
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

                // Recieve a response
                for (const auto& x : recvData())
                        cout << x;
                sendData(formatString("JOIN", channel_));

        } else { // 3 recieves, then send info.
                for (int i = 0; i < 3; ++i) {
                        for (const auto& x : recvData())
                                cout << x;
                }

                if (!pass_.empty())
                        sendData(formatString("PASS", pass_));
                sendData(formatString("NICK", nick_));
                sendData(formatString("USER", nick_));

                for (auto& x : recvData())
                        cout << x;
                sendData(formatString("JOIN", channel_));
        }
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

        if (numRecv==0) { // Connection closed
                cout << "---- No packets received, closing connection. ----"<< endl;
                ret.push_back("DISCONNECT");
                return ret;
        }

        string temp(buf);
        stringstream ss(temp);

        if (lastSize == MAXDATASIZE - 1) { // We had broken sentence
                // Reconstruct broken sentence
                getline(ss,temp,'\n');
                ret.push_back(lastPiece + temp);
                lastPiece = "";
        }

        // Parse the rest
        while(getline(ss, temp, '\n')) { // Chop sentences
                // SURPRISE!!! THERE ARE STILL FUCKING CRs LEFT
                temp.erase(std::remove(temp.begin(), temp.end(), '\r'), temp.end());
                ret.push_back(temp);
        }


        if (numRecv == MAXDATASIZE - 1) {
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
        string ret = formatString("PRIVMSG", channel_);
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
                        string temp = name;
                        temp.erase(temp.size() - 5); // We know the name ends with JOIN

                        // Do this calculation here, not in the realtime loop.
                        string tempWord = word->word_;
                        transform(tempWord.begin(), tempWord.end(), tempWord.begin(), ::tolower);

                        if (tempWord.find(temp) != string::npos) {
                                cout << "Found name match! " << tempWord << " = " << temp << endl;
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
                        users_[i] = users_.back();
                        users_.pop_back();
                }
        }
}

#endif /* Irc_H_ */
