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

#include <atomic>
#include <iostream>
#include <netdb.h>
#include <string>
#include <unistd.h>

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

        atomic<int> socket_; //socket descriptor
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
        string recvData();
        string formatNiceOutput(const string& msg);
        string formatString(const string& command, const string& str);
        string formatPrivMsg(const string& command, const string& channel,
                const string& str);
        void sendPong(const string& msg);
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

        while (true) {
                string msg = recvData();

                // Check ping: http://www.irchelp.org/irchelp/rfc/chapter4.html
                if (msg.find("PING") != string::npos) {
                        sendPong(msg);
                } else if (msg == "DISCONNECT") {
                        return;
                } else if (msg.find("PRIVMSG") != string::npos
                        && msg.find("#") != string::npos) {
                        formatNiceOutput(msg);
                } else {
                        cout << msg;
                }

        }
}

void Irc::say(string const& msg)
{
        sendData(formatPrivMsg("PRIVMSG", channel_, msg));
}



//// PRIVATE ////

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
                cout << recvData();
                sendData(formatString("JOIN", channel_));

        } else { // 3 recieves, then send info.
                cout << recvData() << recvData() << recvData();

                if (!pass_.empty())
                        sendData(formatString("PASS", pass_));
                sendData(formatString("NICK", nick_));
                sendData(formatString("USER", nick_));

                cout << recvData();
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
        if (send(socket_, msg.c_str(), msg.size(), 0) == 0)
                return false;

        //cout << "Sent " << msg.c_str();
        return true;
}

string Irc::recvData()
{
        int numRecv;
        char buf[MAXDATASIZE];
        numRecv = recv(socket_, buf, MAXDATASIZE-1, 0);
        buf[numRecv]='\0';

        if (numRecv==0) { // Connection closed
                cout << "---- No packets received, closing connection. ----"<< endl;
                return string("DISCONNECT");
        }

        return string(buf);
}

string Irc::formatNiceOutput(const string& msg)
{
        string ret, chan;
        ret = msg.substr(msg.find("PRIVMSG"));
        chan = ret.substr(ret.find(":") - 1);
        ret = ret.substr(ret.find(":"));
        ret = ret.erase(0, 1);
        return ret;
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
                cout << msg
                << response << endl;
        }
}

#endif /* Irc_H_ */
