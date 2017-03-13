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
using std::shared_ptr;

class AuctionBot : public AegisModule
{
public:
    AuctionBot(AegisBot & bot, shared_ptr<Guild> guild);
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

    uint64_t currentstandingsid = 0;

    std::vector<uint64_t> admins;
    boost::asio::steady_timer timer;
    uint64_t pausetimeleft = 0;

    uint32_t bidtime = 30000;

    void initialize();
    void remove();

    bool isadmin(const uint64_t id);
    string getparams(const shared_ptr<ABMessage> message);
    string gen_random(const int len);
    void timercontinuation(shared_ptr<Channel> channel);
    Team & getteam(const uint64_t id);

    void Register(shared_ptr<ABMessage> message);
    void Start(shared_ptr<ABMessage> message);
    void End(shared_ptr<ABMessage> message);
    void Playerlist(shared_ptr<ABMessage> message);
    void Nom(shared_ptr<ABMessage> message);
    void Defaultfunds(shared_ptr<ABMessage> message);
    void Pause(shared_ptr<ABMessage> message);
    void Resume(shared_ptr<ABMessage> message);
    void Bid(shared_ptr<ABMessage> message);
    void Setname(shared_ptr<ABMessage> message);
    void Standings(shared_ptr<ABMessage> message);
    void Retain(shared_ptr<ABMessage> message);
    void Skip(shared_ptr<ABMessage> message);
    void Setfunds(shared_ptr<ABMessage> message);
    void Undobid(shared_ptr<ABMessage> message);
    void Bidtime(shared_ptr<ABMessage> message);
    void Adminsetname(shared_ptr<ABMessage> message);
    void Help(shared_ptr<ABMessage> message);
    void Withdraw(shared_ptr<ABMessage> message);
    void Addfunds(shared_ptr<ABMessage> message);
    void Removefunds(shared_ptr<ABMessage> message);
    void Addbidder(shared_ptr<ABMessage> message);
    void Removebidder(shared_ptr<ABMessage> message);
    void Reset(shared_ptr<ABMessage> message);
    void Enable(shared_ptr<ABMessage> message);
    void Disable(shared_ptr<ABMessage> message);
    void Attachments(shared_ptr<ABMessage> message);
};

