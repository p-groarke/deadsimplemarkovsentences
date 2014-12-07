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

#include "irc.h"

#include <iostream>
#include <netdb.h>
#include <unistd.h>

#define MAXDATASIZE 1000


Irc::Irc(const string& nick, const string& usr, const string& address,
        const string& channel, const string& pass, const string& port) :
        nick_(nick),
        usr_(usr),
        address_(address),
        channel_(channel),
        pass_(pass),
        port_(port)
{}

Irc::Irc(const Irc& obj) :
        nick_(obj.nick_),
        usr_(obj.usr_),
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
                } else { //Pass buf to the message handeler
                        outputMsg(msg);
                }

        }
}

void Irc::say(string const& msg)
{
        string tempChan = channel_.substr(5);
        tempChan = tempChan.erase(tempChan.size() - 2, tempChan.size());
        sendData("PRIVMSG #socapex :" + msg + "\r\n");
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

                sendData(pass_);
                sendData(nick_);
                sendData(usr_);

                // Recieve a response
                cout << recvData();
                sendData(channel_);

        } else { // 3 recieves, then send info.
                cout << recvData() << recvData() << recvData();
                sendData(nick_);
                sendData(usr_);
                cout << recvData();
                sendData(channel_);
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

void Irc::sendPong(const string& msg)
{
        string response = "PONG " + msg.substr(5);

        if (sendData(response)) {
                cout << msg
                << response << endl;
        }
}

/*
* TODO: add you code to respod to commands here
* the example below replys to the command hi scooby
*/
void Irc::outputMsg(const string& msg)
{
        cout << msg;
}

