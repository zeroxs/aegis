//
// AuctionBot.h
// aegisbot
//
// Copyright (c) 2017 Zero (zero at xandium dot net)
//
// This file is part of aegisbot.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <string>
#include <stdint.h>
#include <vector>
#include <boost/asio/steady_timer.hpp>
#include "AegisModule.h"

class ABMessage;
class Channel;
class AegisBot;

using std::string;

class AuctionBot : public AegisModule
{
public:
    AuctionBot(AegisBot & bot, Guild & guild);
    ~AuctionBot() {};

    struct Team
    {
        int id;
        string owner;
        uint64_t owner_id;
        std::vector<uint64_t> bidders;
        string teamname;
        std::vector<std::pair<string, int>> players;
        int funds = 0;
        bool withdrawn = false;
    };

    std::vector<std::pair<string,bool>> players;
    std::vector<Team> teams;
    int defaultfunds = 0;
    int direction = 1;
    int lastteam = 0;
    int currentteam = 0;
    bool auctioninprogress = false;
    string currentnom;
    std::vector<std::pair<int, int>> bids;//team id, bid amount
    bool paused = false;
    uint64_t timeuntilstop = 0;
    uint64_t channeloutput = 0;

    uint64_t currentstandingsid = 0;

    std::vector<uint64_t> admins;
    boost::asio::steady_timer timer;
    uint64_t pausetimeleft = 0;

    uint32_t bidtime = 30000;

    void initialize();
    void remove();
    bool nextplayer();

    bool isadmin(const uint64_t id);
    string getparams(ABMessage & message);
    string gen_random(const int len);
    void timercontinuation(Channel * channel);
    Team & getteam(const uint64_t id);

    void Register(ABMessage & message);
    void Start(ABMessage & message);
    void End(ABMessage & message);
    void Playerlist(ABMessage & message);
    void Nom(ABMessage & message);
    void Defaultfunds(ABMessage & message);
    void Pause(ABMessage & message);
    void Resume(ABMessage & message);
    void Bid(ABMessage & message);
    void Setname(ABMessage & message);
    void Standings(ABMessage & message);
    void Retain(ABMessage & message);
    void Skip(ABMessage & message);
    void Setfunds(ABMessage & message);
    void Undobid(ABMessage & message);
    void Bidtime(ABMessage & message);
    void Adminsetname(ABMessage & message);
    void Help(ABMessage & message);
    void Withdraw(ABMessage & message);
    void Addfunds(ABMessage & message);
    void Removefunds(ABMessage & message);
    void Addbidder(ABMessage & message);
    void Removebidder(ABMessage & message);
    void Reset(ABMessage & message);
    void Enable(ABMessage & message);
    void Disable(ABMessage & message);
    void Attachments(ABMessage & message);
};

