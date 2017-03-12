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
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

AuctionBot::AuctionBot()
    : timer(AegisBot::GetSingleton().io_service)
{

}


AuctionBot::~AuctionBot()
{
}

void AuctionBot::initialize()
{
    admins.push_back(159046292633419776LL);//Disaster
    admins.push_back(171000788183678976LL);//Rensia

    for (int i = 0; i < 120; ++i)
        players.push_back({ Poco::format("Player %d", i), true });

    AegisBot & bot = AegisBot::GetSingleton();

    auto guild = bot.CreateGuild(289234114580840448LL);

    guild->addCommand("register", std::bind(&AuctionBot::Register, this, std::placeholders::_1));
    guild->addCommand("start", std::bind(&AuctionBot::Start, this, std::placeholders::_1));
    guild->addCommand("playerlist", std::bind(&AuctionBot::Playerlist, this, std::placeholders::_1));
    guild->addCommand("nom", std::bind(&AuctionBot::Nom, this, std::placeholders::_1));
    guild->addCommand("nominate", std::bind(&AuctionBot::Nom, this, std::placeholders::_1));
    guild->addCommand("defaultfunds", std::bind(&AuctionBot::Defaultfunds, this, std::placeholders::_1));
    guild->addCommand("pause", std::bind(&AuctionBot::Pause, this, std::placeholders::_1));
    guild->addCommand("resume", std::bind(&AuctionBot::Resume, this, std::placeholders::_1));
    guild->addCommand("bid", std::bind(&AuctionBot::Bid, this, std::placeholders::_1));
    guild->addCommand("b", std::bind(&AuctionBot::Bid, this, std::placeholders::_1));
    guild->addCommand("end", std::bind(&AuctionBot::End, this, std::placeholders::_1));
    guild->addCommand("setname", std::bind(&AuctionBot::Setname, this, std::placeholders::_1));
    guild->addCommand("standings", std::bind(&AuctionBot::Standings, this, std::placeholders::_1));
    guild->addCommand("retain", std::bind(&AuctionBot::Retain, this, std::placeholders::_1));
    guild->addCommand("skip", std::bind(&AuctionBot::Skip, this, std::placeholders::_1));
    guild->addCommand("setfunds", std::bind(&AuctionBot::Setfunds, this, std::placeholders::_1));
    guild->addCommand("undobid", std::bind(&AuctionBot::Undobid, this, std::placeholders::_1));
    guild->addCommand("bidtime", std::bind(&AuctionBot::Bidtime, this, std::placeholders::_1));
    guild->addCommand("fsetname", std::bind(&AuctionBot::Adminsetname, this, std::placeholders::_1));
    guild->addCommand("help", std::bind(&AuctionBot::Help, this, std::placeholders::_1));
    guild->addCommand("withdraw", std::bind(&AuctionBot::Withdraw, this, std::placeholders::_1));
    guild->addCommand("addfunds", std::bind(&AuctionBot::Addfunds, this, std::placeholders::_1));
    guild->addCommand("removefunds", std::bind(&AuctionBot::Removefunds, this, std::placeholders::_1));
    guild->addCommand("addbidder", std::bind(&AuctionBot::Addbidder, this, std::placeholders::_1));
    guild->addCommand("removebidder", std::bind(&AuctionBot::Removebidder, this, std::placeholders::_1));
    guild->addCommand("reset", std::bind(&AuctionBot::Reset, this, std::placeholders::_1));
    guild->addCommand("enable", std::bind(&AuctionBot::Enable, this, std::placeholders::_1));
    guild->addCommand("disable", std::bind(&AuctionBot::Disable, this, std::placeholders::_1));

    guild->attachmentHandler(std::bind(&AuctionBot::Attachments, this, std::placeholders::_1));
}

void AuctionBot::Attachments(shared_ptr<ABMessage> message)
{
    json url = message->obj["attachments"];
}

void AuctionBot::Reset(shared_ptr<ABMessage> message)
{
    if (!isadmin(message->member->id))
        return;

    teams.clear();
    currentnom = "";
    currentteam = 0;
    players.clear();
    bids.clear();
    message->channel->sendMessage("Data Reset.");
}

bool AuctionBot::isadmin(const uint64_t id)
{
    for (auto & i : admins)
        if (i == id)
            return true;
    return false;
}

AuctionBot::Team & AuctionBot::getteam(const uint64_t id)
{
    for (auto & t : teams)
    {
        if (t.owner_id == id)
        {
            return t;
        }
    }
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
    if (auctioninprogress)
    {
        if (!paused)
            channel->sendMessage(Poco::format("Auction for player **%s** currently at [%d] by **%s**", currentnom, bids.back().second, teams[bids.back().first].teamname));
        if (timeuntilstop <= std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
        {
            auto res = bids.back();
            //award player

            teams[res.first].funds -= res.second;
            teams[res.first].players.push_back({ currentnom, res.second });


            do 
            {
                lastteam = currentteam;

                currentteam += direction;

                if (currentteam > teams.size() - 1)
                    currentteam = teams.size() - 1;
                if (currentteam < 0)
                    currentteam = 0;

                if (currentteam == lastteam)
                {
                    if (direction == -1) direction = 1;
                    else if (direction == 1) direction = -1;
                }

                if (!teams[currentteam].withdrawn)
                    break;
            } while (1);


            json jteams;
            for (auto & t : teams)
            {
                std::stringstream players;
                for (auto & p : t.players)
                    players << p.first << " (" << p.second << ")\n";
                jteams.push_back(json({ { "name", Poco::format("[%d] %s (%d)", t.id, t.teamname, t.funds) },{ "value", players.str() == "" ? "No players purchased yet" : players.str() },{ "inline", true } }));
            }
            json t = {
                { "title", "Current Standings" },
                { "color", 10599460 },
                { "fields", jteams },
                { "footer",{ { "icon_url", "https://cdn.discordapp.com/attachments/288707540844412928/289572000391888906/cpp.png" },{ "text", "Auction bot" } } }
            };
            channel->sendMessageEmbed(Poco::format("Auction of player **%s** completed for [%d] awarded to **%s**\n<@%Lu> type `%snom player name` to nominate another player.", currentnom, res.second, teams[bids.back().first].teamname, teams[currentteam].owner_id, channel->belongs_to()->prefix), t);


            for (int i = 0; i < players.size(); ++i)
            {
                if (players[i].first == currentnom)
                {
                    players[i].second = false;
                }
            }

            return;
        }
        timer.expires_from_now(std::chrono::milliseconds(5000));
        timer.async_wait(std::bind(&AuctionBot::timercontinuation, this, AegisBot::GetSingleton().channellist[289234114580840448LL]));
    }
}


void AuctionBot::Register(shared_ptr<ABMessage> message)
{
    if (auctioninprogress)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] Auction already in progress", message->member->id));
        return;
    }
    string name = getparams(message);

    for (auto & t : teams)
    {
        if (t.owner_id == message->member->id)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Command failed. You are already registered", message->member->id));
            return;
        }
    }
    Team t;
    t.funds = defaultfunds;
    t.owner = message->member->name;
    t.owner_id = message->member->id;
    t.id = teams.size();
    if (name.size() > 0)
    {
        t.teamname = name;
        message->channel->sendMessage(Poco::format("[<@%Lu>] Registered for auction successfully. Team name **%s**", message->member->id, name));
        t.bidders.push_back(message->member->id);
    }
    else
        message->channel->sendMessage(Poco::format("[<@%Lu>] Registered for auction successfully. Set your team name with `%ssetname name here`", message->member->id, message->guild->prefix));
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
        message->channel->sendMessage(Poco::format("Auction has begun. First team to nominate **%s** type `%snom player name`", teams[currentteam].teamname, message->guild->prefix));
        auctioninprogress = true;
    }
}

void AuctionBot::Playerlist(shared_ptr<ABMessage> message)
{
    std::vector<std::stringstream> outputs;
    for (auto & t : {1,2,3,4,5,6})
    {
        outputs.push_back(std::stringstream());
    }

    json jteams;

    for (int i = 0; i < players.size(); ++i)
    {
        if (players[i].second)//taken
            outputs[i%6] << players[i].first << "\n";
//         else
//             outputs[i % 6] << "~~" << players[i].first << "~~\n";
    }
    for (int i = 0; i < outputs.size(); ++i)
    {
        jteams.push_back(json({ { "name", "Players" },{ "value", outputs[i].str().size()>0? outputs[i].str():"No Players" },{ "inline", true } }));
    }
    json t = {
        { "title", "Current Players" },
        { "color", 10599460 },
        { "fields", jteams },
        { "footer",{ { "icon_url", "https://cdn.discordapp.com/attachments/288707540844412928/289572000391888906/cpp.png" },{ "text", "Auction bot" } } }
    };
    message->channel->sendMessageEmbed("", t);


}

void AuctionBot::Nom(shared_ptr<ABMessage> message)
{
    if (!auctioninprogress)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] No auction going on.", message->member->id));
        return;
    }

    if (message->member->id != teams[currentteam].owner_id)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] It is not your turn to nominate.", message->member->id));
        return;
    }
    for (auto & b : teams[currentteam].bidders)
    {
        std::cout << "Allowed bidder: " << b << std::endl;
    }
    try
    {
        for (auto & b : teams[currentteam].bidders)
        {
            if (b == message->member->id)
            {
                string name = getparams(message);
                if (name.size() == 0)
                {
                    message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
                    return;
                }
                for (auto & p : players)
                {
                    if (p.first == name)
                    {
                        if (!p.second)
                        {
                            message->channel->sendMessage(Poco::format("[<@%Lu>] Player [%s] already bought.", message->member->id, name));
                            return;
                        }
                        currentnom = p.first;
                        bids.clear();
                        bids.push_back({ getteam(message->member->id).id, 3000 });
                        message->channel->sendMessage(Poco::format("Auction started for player **%s** Current bid at [%d] To bid, type `%sbid value` Only increments of 500 allowed.", p.first, bids.back().second, message->guild->prefix));
                        timeuntilstop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + bidtime;
                        timer.expires_from_now(std::chrono::milliseconds(5000));
                        timer.async_wait(std::bind(&AuctionBot::timercontinuation, this, AegisBot::GetSingleton().channellist[289234114580840448LL]));
                        return;
                    }
                }
                return;
            }
        }
    }
    catch (...)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
    }
}

void AuctionBot::Defaultfunds(shared_ptr<ABMessage> message)
{
    if (isadmin(message->member->id))
    {
        string bid = getparams(message);
        if (bid.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }
        int dbid = std::stoi(bid);;
        for (auto & t : teams)
            t.funds = dbid;
        message->channel->sendMessage(Poco::format("[<@%Lu>] Starting funds and all current teams set to [%d]", message->member->id, dbid));
        defaultfunds = std::stoi(bid);
    }
}

void AuctionBot::Pause(shared_ptr<ABMessage> message)
{
    if (!auctioninprogress)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] No auction going on.", message->member->id));
        return;
    }

    if (isadmin(message->member->id))
    {
        pausetimeleft = timeuntilstop - std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        message->channel->sendMessage("Auction has been paused.");
        paused = true;
    }
}

void AuctionBot::Resume(shared_ptr<ABMessage> message)
{
    if (!auctioninprogress)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] No auction going on.", message->member->id));
        return;
    }

    if (isadmin(message->member->id))
    {
        timeuntilstop += pausetimeleft;
        message->channel->sendMessage("Auction has been resumed.");
        paused = false;
    }
}

void AuctionBot::Bid(shared_ptr<ABMessage> message)
{
    if (!auctioninprogress)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] No auction going on.", message->member->id));
        return;
    }

    string sbid = getparams(message);
    if (sbid.size() == 0)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
        return;
    }
    int bid = std::stoi(sbid);

    if (bid % 500 > 0)
    {
        //message->channel->sendMessage(Poco::format("DEBUG [<@%Lu>] Bid not a multiple of 500 [%d].", message->member->id, bid));
        return;
    }

    for (auto & t : teams)
    {
        for (auto & b : t.bidders)
        {
            if (b == message->member->id)
            {
                if (t.withdrawn)
                    return;
                if (bid > bids.back().second)
                {
                    if (bid > t.funds - ((10 - t.players.size()) * 3000) || bid > t.funds)
                    {
                        message->channel->sendMessage(Poco::format("[<@%Lu>] Bid is too high for your funds [%d].", message->member->id, t.funds));
                        return;
                    }

                    bids.push_back({ t.id, bid });
                    //message->channel->sendMessage(Poco::format("DEBUG [<@%Lu>] Bid increased to [%d].", message->member->id, bids.back().second));
                    timeuntilstop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + bidtime;
                    return;
                }
                else
                {
                    //message->channel->sendMessage(Poco::format("DEBUG [<@%Lu>] Bid not large enough. Current price [%d].", message->member->id, bids.back().second));
                    return;
                }
            }
        }
    }
    message->channel->sendMessage(Poco::format("[<@%Lu>] You do not have a team to bid for.", message->member->id));
}

void AuctionBot::End(shared_ptr<ABMessage> message)
{
    if (!auctioninprogress)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] No auction going on.", message->member->id));
        return;
    }

    if (message->member->id == 159046292633419776LL || message->member->id == 171000788183678976LL)
    {
        currentteam = 0;
        json jteams;
        for (auto & t : teams)
        {
            std::stringstream players;
            for (auto & p : t.players)
                players << p.first << " (" << p.second << ")\n";
            jteams.push_back(json({ { "name", Poco::format("%s (%d)", t.teamname, t.funds) },{ "value", players.str() == "" ? "No players won yet" : players.str() },{ "inline", true } }));
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
    string name = getparams(message);
    for (auto & t : teams)
    {
        if (t.owner_id == message->member->id)
        {
            t.teamname = name;
            message->channel->sendMessage(Poco::format("[<@%Lu>] Name set successfully. **%s**", message->member->id, name));
            return;
        }
    }
    message->channel->sendMessage(Poco::format("[<@%Lu>] You are not registered yet. Register for the auction with `%sregister`", message->member->id, message->guild->prefix));
}

void AuctionBot::Standings(shared_ptr<ABMessage> message)
{
    json jteams;
    for (auto & t : teams)
    {
        std::stringstream players;
        for (auto & p : t.players)
            players << p.first << " (" << p.second << ")\n";
        jteams.push_back(json({ { "name", Poco::format("[%d] %s (%d)", t.id+1, t.teamname, t.funds) },{ "value", players.str() == "" ? "No players purchased yet" : players.str() }, { "inline", true } }));
    }
    json t = {
        { "title", "Current Standings" },
        { "color", 10599460 },
        { "fields", jteams },
        { "footer",{ { "icon_url", "https://cdn.discordapp.com/attachments/288707540844412928/289572000391888906/cpp.png" },{ "text", "Auction bot" } } }
    };
    message->channel->sendMessageEmbed("", t);
}

void AuctionBot::Retain(shared_ptr<ABMessage> message)
{
    if (!isadmin(message->member->id))
        return;
    try
    {
        std::vector<string> tokens;
        boost::split(tokens, message->content, boost::is_any_of(" "));

        int len = 0;

        if (tokens.size() < 4)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }

        len += tokens[0].size();//cmd
        len += tokens[1].size();//team index
        len += tokens[2].size();//funds

        len += 3;

        string playername = message->content.substr(len);
        int teamindex = std::stoi(tokens[1])-1;
        int funds = std::stoi(tokens[2]);

        if (teamindex > teams.size())
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Team does not exist.", message->member->id));
            return;
        }


        for (auto & p : players)
        {
            if (p.first == playername)
            {
                p.second = false;
                teams[teamindex].funds -= funds;
                teams[teamindex].players.push_back({ playername, funds });
                message->channel->sendMessage(Poco::format("[<@%Lu>] Player **%s** retained to team **%s** for [%d]", message->member->id, playername, teams[teamindex].teamname, funds));
                return;
            }
        }

    }
    catch (...)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
    }
}

void AuctionBot::Skip(shared_ptr<ABMessage> message)
{
    if (!isadmin(message->member->id))
        return;
    if (!auctioninprogress)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] No auction going on.", message->member->id));
        return;
    }
    int failurecheck = 0;
    string oldteam = teams[currentteam].teamname;
    do
    {
        lastteam = currentteam;

        currentteam += direction;

        if (currentteam < 0)
            currentteam = 0;
        if (currentteam > teams.size() - 1)
            currentteam = teams.size() - 1;


        if (currentteam == lastteam)
        {
            if (direction == -1) direction = 1;
            else if (direction == 1) direction = -1;
        }

        if (!teams[currentteam].withdrawn)
            break;
        failurecheck++;
    } while (failurecheck < 15);

    message->channel->sendMessage(Poco::format("[%s]'s turn skipped\n[%s] <@%Lu> type `%snom player name` to nominate another player.", oldteam, teams[currentteam].teamname, teams[currentteam].owner_id, message->channel->belongs_to()->prefix));
}

void AuctionBot::Setfunds(shared_ptr<ABMessage> message)
{
    if (!isadmin(message->member->id))
        return;
    try
    {
        boost::char_separator<char> sep{ " " };
        boost::tokenizer<boost::char_separator<char>> tok{ message->content, sep };

        auto token = tok.begin();

        token++;

        string team = *token++;

        if (team.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }

        if (teams.size() < std::stoi(team)-1)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid team.", message->member->id));
            return;
        }

        string funds = *token++;

        if (funds.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }


        teams[std::stoi(team)-1].funds = std::stoi(funds);
        message->channel->sendMessage(Poco::format("[<@%Lu>] Funds for **%s** set successfully. [%d]", message->member->id, teams[std::stoi(team)-1].teamname, teams[std::stoi(team)-1].funds));
    }
    catch (...)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
    }
}

void AuctionBot::Undobid(shared_ptr<ABMessage> message)
{
    if (!isadmin(message->member->id))
        return;
    if (!auctioninprogress)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] No auction going on.", message->member->id));
        return;
    }

    if (bids.size() > 1)
    {
        bids.pop_back();
        message->channel->sendMessage(Poco::format("[<@%Lu>] Undo bid successful. Current Bid [%d] by **%s**.", message->member->id, bids.back().second, teams[bids.back().first].teamname));
        return;
    }
    else
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] There is no last bidder.", message->member->id));
        return;
    }
}

void AuctionBot::Bidtime(shared_ptr<ABMessage> message)
{
    if (isadmin(message->member->id))
    {
        string bid = getparams(message);
        if (bid.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }
        bidtime = std::stoi(bid)*1000;
        message->channel->sendMessage(Poco::format("[<@%Lu>] Time between bids set to [%u]", message->member->id, bidtime));
    }
}

void AuctionBot::Help(shared_ptr<ABMessage> message)
{
    json admincommands = json({ { "name", "Admin only" },{ "value", "start, end, defaultfunds, pause, resume, setfunds, bidtime, fsetname, retain, skip, undobid, reset" },{ "inline", true } });
    json usercommands = json({ { "name", "Manager" },{ "value", "register, playerlist, nom, b, bid, setname, standings" },{ "inline", true } });
    json t = {
        { "title", "Commands" },
        { "color", 10599460 },
        { "fields", { admincommands, usercommands } },
        { "footer",{ { "icon_url", "https://cdn.discordapp.com/attachments/288707540844412928/289572000391888906/cpp.png" },{ "text", "Auction bot" } } }
    };
    message->channel->sendMessageEmbed("", t);
}

void AuctionBot::Withdraw(shared_ptr<ABMessage> message)
{
    if (!auctioninprogress)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] No auction going on.", message->member->id));
        return;
    }
    for (auto & t : teams)
    {
        for (auto & b : t.bidders)
        {
            if (b == message->member->id)
            {
                if (t.players.size() < 10)
                {
                    message->channel->sendMessage(Poco::format("[<@%Lu>] Unable to withdraw. Need at least 10 players. Your players: **%d**.", message->member->id, t.players.size()));
                    return;
                }
                message->channel->sendMessage(Poco::format("[<@%Lu>] Your team **%s** has withdrawn from the auction.", message->member->id, t.teamname));
                t.withdrawn = true;
                return;
            }
        }
    }
}

void AuctionBot::Addfunds(shared_ptr<ABMessage> message)
{
    if (!isadmin(message->member->id))
        return;
    try
    {
        boost::char_separator<char> sep{ " " };
        boost::tokenizer<boost::char_separator<char>> tok{ message->content, sep };

        auto token = tok.begin();

        token++;

        string team = *token++;

        if (team.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }

        if (teams.size() < std::stoi(team) - 1)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid team.", message->member->id));
            return;
        }

        string funds = *token++;

        if (funds.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }

        teams[std::stoi(team) - 1].funds += std::stoi(funds);
        message->channel->sendMessage(Poco::format("[<@%Lu>] Funds for **%s** set successfully. +%d [%d]", message->member->id, teams[std::stoi(team) - 1].teamname, std::stoi(funds), teams[std::stoi(team) - 1].funds));
    }
    catch (...)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
    }
}

void AuctionBot::Removefunds(shared_ptr<ABMessage> message)
{
    if (!isadmin(message->member->id))
        return;
    try
    {
        boost::char_separator<char> sep{ " " };
        boost::tokenizer<boost::char_separator<char>> tok{ message->content, sep };

        auto token = tok.begin();

        token++;

        string team = *token++;

        if (team.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }

        if (teams.size() < std::stoi(team) - 1)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid team.", message->member->id));
            return;
        }

        string funds = *token++;

        if (funds.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }

        teams[std::stoi(team) - 1].funds -= std::stoi(funds);
        message->channel->sendMessage(Poco::format("[<@%Lu>] Funds for **%s** set successfully. -%d [%d]", message->member->id, teams[std::stoi(team) - 1].teamname, std::stoi(funds), teams[std::stoi(team) - 1].funds));
    }
    catch (...)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
    }
}

void AuctionBot::Adminsetname(shared_ptr<ABMessage> message)
{
    if (!isadmin(message->member->id))
        return;
    try
    {
        boost::char_separator<char> sep{ " " };
        boost::tokenizer<boost::char_separator<char>> tok{ message->content, sep };

        int length = 0;

        auto token = tok.begin();
        length += (*token).size();
        token++;

        string team = *token++;

        if (team.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }

        if (teams.size() < std::stoi(team)-1)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid team.", message->member->id));
            return;
        }

        string name = message->content.substr(length + 2);
        //string name = *token++;

        if (name.size() == 0)
        {
            message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
            return;
        }

        string oldname = teams[std::stoi(team) - 1].teamname;
        teams[std::stoi(team)-1].teamname = name;
        message->channel->sendMessage(Poco::format("[<@%Lu>] Name successfully changed from **%s** to **%s**", message->member->id, oldname, teams[std::stoi(team)-1].teamname));
    }
    catch (...)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
    }
}

void AuctionBot::Addbidder(shared_ptr<ABMessage> message)
{
    string bidder = getparams(message);
    if (bidder.size() == 0)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
        return;
    }
    uint64_t addbidder = std::stoull(bidder.substr(2, bidder.size()-3));

    for (auto & t : teams)
    {
        if (t.owner_id == message->member->id)
        {
            if (t.withdrawn)
                return;
            t.bidders.push_back(addbidder);
            message->channel->sendMessage(Poco::format("[<@%Lu>] Added [<@%Lu>] to **%s**'s list of bidders.", message->member->id, addbidder, t.teamname));
            return;
        }
    }
    message->channel->sendMessage(Poco::format("[<@%Lu>] You are not the manager of a team.", message->member->id));
}

void AuctionBot::Removebidder(shared_ptr<ABMessage> message)
{
    string bidder = getparams(message);
    if (bidder.size() == 0)
    {
        message->channel->sendMessage(Poco::format("[<@%Lu>] Invalid command arguments.", message->member->id));
        return;
    }
    uint64_t rembidder = std::stoull(bidder.substr(2, bidder.size() - 3));

    for (auto & t : teams)
    {
        if (t.owner_id == message->member->id)
        {
            if (t.withdrawn)
                return;
            for (auto it = t.bidders.begin(); it != t.bidders.end(); ++it)
            {
                if (*it == rembidder)
                {
                    t.bidders.erase(it);
                    message->channel->sendMessage(Poco::format("[<@%Lu>] Removed [<@%Lu>] from **%s**'s list of bidders.", message->member->id, rembidder, t.teamname));
                    return;
                }
            }
        }
    }
    message->channel->sendMessage(Poco::format("[<@%Lu>] You are not the manager of a team.", message->member->id));
}

void AuctionBot::Enable(shared_ptr<ABMessage> message)
{
    for (std::pair<const string, ABCallbackPair> & c : AegisBot::GetSingleton().guildlist[289234114580840448LL]->cmdlist)
    {
        c.second.first.enabled = true;
        c.second.first.level = 0;
    }
    message->channel->sendMessage("Auction commands enabled");
}

void AuctionBot::Disable(shared_ptr<ABMessage> message)
{
    for (std::pair<const string, ABCallbackPair> & c : AegisBot::GetSingleton().guildlist[289234114580840448LL]->cmdlist)
    {
        if (c.first != "enable")
        {
            c.second.first.enabled = false;
            c.second.first.level = 1;
        }
    }
    message->channel->sendMessage("Auction commands disabled");

}
