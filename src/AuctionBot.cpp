//
// AuctionBot.cpp
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

#include "AuctionBot.h"
#include "Channel.h"
#include "Member.h"
#include "Guild.h"
#include "AegisBot.h"

AuctionBot::AuctionBot()
{

}


AuctionBot::~AuctionBot()
{
}

void AuctionBot::initialize()
{
    admins.push_back(159046292633419776LL);//Disaster
    admins.push_back(171000788183678976LL);//Rensia

    for (int i = 0; i < 60; ++i)
        players.push_back(Poco::format("Player %d", i));

    AegisBot & bot = AegisBot::GetSingleton();

    auto guild = bot.CreateGuild(289234114580840448LL);

    guild->addCommand("register", std::bind(&AuctionBot::Register, this, std::placeholders::_1));
    guild->addCommand("start", std::bind(&AuctionBot::Start, this, std::placeholders::_1));
    guild->addCommand("playerlist", std::bind(&AuctionBot::Playerlist, this, std::placeholders::_1));
    guild->addCommand("nom", std::bind(&AuctionBot::Nom, this, std::placeholders::_1));
    guild->addCommand("defaultfunds", std::bind(&AuctionBot::Defaultfunds, this, std::placeholders::_1));
    guild->addCommand("pause", std::bind(&AuctionBot::Pause, this, std::placeholders::_1));
    guild->addCommand("resume", std::bind(&AuctionBot::Resume, this, std::placeholders::_1));
    guild->addCommand("bid", std::bind(&AuctionBot::Bid, this, std::placeholders::_1));
    guild->addCommand("end", std::bind(&AuctionBot::End, this, std::placeholders::_1));
    guild->addCommand("setname", std::bind(&AuctionBot::Setname, this, std::placeholders::_1));
    guild->addCommand("standings", std::bind(&AuctionBot::Standings, this, std::placeholders::_1));
}

bool AuctionBot::isadmin(const uint64_t id)
{
    for (auto & i : admins)
        if (i == id)
            return true;
    return false;
}

string AuctionBot::getparams(const shared_ptr<ABMessage> message)
{
    if (message->content.size() > message->guild->prefix.size() + message->cmd.size() + 1)
        return message->content.substr(message->guild->prefix.size() + message->cmd.size() + 1);
    return "";
}

string AuctionBot::gen_random(const int len)
{
    std::stringstream ss;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i)
        ss << alphanum[rand() % (sizeof(alphanum) - 1)];
    return ss.str();
}

void AuctionBot::timercontinuation(shared_ptr<Channel> channel)
{
    if (!paused)
        channel->sendMessage(Poco::format("Auction for player [%s]", currentnom));
    if (auctioninprogress)
    {
        if (timeuntilstop <= std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
        {
            //award player
            return;
        }
        timer->async_wait(std::bind(&AuctionBot::timercontinuation, this, AegisBot::GetSingleton().channellist[289234114580840448LL]));
    }
}


void AuctionBot::Register(shared_ptr<ABMessage> message)
{
    string name = getparams(message);

    for (auto & t : teams)
    {
        if (t.owner_id == message->member->id)
        {
            message->channel->sendMessage(Poco::format("[%s] Command failed. You are already registered", message->member->name));
            return;
        }
    }
    Team t;
    t.funds = defaultfunds;
    t.owner = message->member->name;
    t.owner_id = message->member->id;
    if (name.size() > 0)
    {
        t.teamname = name;
        message->channel->sendMessage(Poco::format("[%s] Registered for auction successfully. Team name [%s]", message->member->name, name));
    }
    else
        message->channel->sendMessage(Poco::format("[%s] Registered for auction successfully. Set your team name with `%ssetname name here`", message->member->name, message->guild->prefix));
    teams.push_back(t);
}

void AuctionBot::Start(shared_ptr<ABMessage> message)
{
    if (isadmin(message->member->id))
    {
        if (teams.size() == 0)
        {
            message->channel->sendMessage("Team list empty");
            return;
        }
        currentteam = 0;
        message->channel->sendMessage(Poco::format("Auction has begun. First team to nominate [%s] type `%snom player name`", teams[currentteam].teamname, message->guild->prefix));
        auctioninprogress = true;
    }
}

void AuctionBot::Playerlist(shared_ptr<ABMessage> message)
{
    std::stringstream ss;
    for (auto & player : players)
    {
        ss << "\n" << player;
    }
    message->channel->sendMessage(Poco::format("Player list: %s", ss.str()));
}

void AuctionBot::Nom(shared_ptr<ABMessage> message)
{
    try
    {
        string name = getparams(message);
        if (name.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[%s] Invalid command arguments.", message->member->name));
            return;
        }
        for (auto & p : players)
        {
            if (p == name)
            {
                currentnom = p;
                currentbid = 3000;
                message->channel->sendMessage(Poco::format("Auction started for player [%s] Current bid at [%d] To bid, type `%sbid value` Only increments of 500 allowed.", p, currentbid, message->guild->prefix));
                timeuntilstop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 15000;
                timer->expires_from_now(std::chrono::milliseconds(5000));
                timer->async_wait(std::bind(&AuctionBot::timercontinuation, this, AegisBot::GetSingleton().channellist[289234114580840448LL]));
            }
        }
    }
    catch (...)
    {
        message->channel->sendMessage(Poco::format("[%s] Invalid command arguments.", message->member->name));
    }
}

void AuctionBot::Defaultfunds(shared_ptr<ABMessage> message)
{
    if (isadmin(message->member->id))
    {
        string bid = getparams(message);
        if (bid.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[%s] Invalid command arguments.", message->member->name));
            return;
        }
        int dbid = std::stoi(bid);;
        for (auto & t : teams)
            t.funds = dbid;
        message->channel->sendMessage(Poco::format("[%s] Starting funds and all current teams set to [%d]", message->member->name, dbid));
        defaultfunds = std::stoi(bid);
    }
}

void AuctionBot::Pause(shared_ptr<ABMessage> message)
{
    if (isadmin(message->member->id))
    {
        message->channel->sendMessage("Auction has been paused.");
        paused = true;
    }
}

void AuctionBot::Resume(shared_ptr<ABMessage> message)
{
    if (isadmin(message->member->id))
    {
        message->channel->sendMessage("Auction has been resumed.");
        paused = false;
    }
}

void AuctionBot::Bid(shared_ptr<ABMessage> message)
{
    string sbid = getparams(message);
    if (sbid.size() == 0)
    {
        message->channel->sendMessage(Poco::format("[%s] Invalid command arguments.", message->member->name));
        return;
    }
    int bid = std::stoi(sbid);

    if (bid % 500 > 0)
    {
        message->channel->sendMessage(Poco::format("DEBUG [%s] Bid not a multiple of 500 [%d].", message->member->name, bid));
        return;
    }

    for (auto & t : teams)
    {
        if (t.owner_id == message->member->id)
        {
            if (bid > currentbid)
            {
                currentbid = bid;
                message->channel->sendMessage(Poco::format("DEBUG [%s] Bid increased to [%d].", message->member->name, currentbid));
                return;
            }
            else
            {
                message->channel->sendMessage(Poco::format("DEBUG [%s] Bid not large enough. Current price [%d].", message->member->name, currentbid));
                return;
            }
        }
    }
    message->channel->sendMessage(Poco::format("DEBUG [%s] You do not have a team to bid for.", message->member->name));
}

void AuctionBot::End(shared_ptr<ABMessage> message)
{
    if (message->member->id == 159046292633419776LL || message->member->id == 171000788183678976LL)
    {
        currentteam = 0;
        json jteams;
        for (auto & t : teams)
        {
            std::stringstream players;
            for (auto & p : t.players)
                players << p.first << " (" << p.second << ")";
            jteams.push_back(json({ { "name", Poco::format("%s (%d)", t.teamname, t.funds) },{ "value", players.str() == "" ? "No players won yet" : players.str() } }));
        }
        if (jteams.empty())
        {
            jteams.push_back(json({ { "name", "Empty" },{ "value", "Empty" } }));
        }
        json t = {
            { "title", "Current Standings" },
            { "color", 12330144 },
            { "fields", jteams },
            { "footer",{ { "icon_url", "https://cdn.discordapp.com/attachments/288707540844412928/289572000391888906/cpp.png" },{ "text", "Auction bot" } } }
        };
        message->channel->sendMessageEmbed("Auction has ended. Current standings:\n\n", t);
        auctioninprogress = false;
    }
}

void AuctionBot::Setname(shared_ptr<ABMessage> message)
{
    string name = message->content.substr(message->guild->prefix.size() + message->cmd.size() + 1);
    for (auto & t : teams)
    {
        if (t.owner_id == message->member->id)
        {
            t.teamname = name;
            message->channel->sendMessage(Poco::format("[%s] Name set successfully. [%s]", message->member->name, name));
            return;
        }
    }
    message->channel->sendMessage(Poco::format("[%s] You are not registered yet. Register for the auction with `%sregister`", message->member->name, message->guild->prefix));
}

void AuctionBot::Standings(shared_ptr<ABMessage> message)
{
    json jteams;
    for (auto & t : teams)
    {
        std::stringstream players;
        for (auto & p : t.players)
            players << p.first << " (" << p.second << ")";
        jteams.push_back(json({ { "name", Poco::format("%s (%d)", t.teamname, t.funds) },{ "value", players.str() == "" ? "No players purchased yet" : players.str() } }));
    }
    json t = {
        { "title", "Current Standings" },
        { "color", 10599460 },
        { "fields", jteams },
        { "footer",{ { "icon_url", "https://cdn.discordapp.com/attachments/288707540844412928/289572000391888906/cpp.png" },{ "text", "Auction bot" } } }
    };
    message->channel->sendMessageEmbed("", t);
}

