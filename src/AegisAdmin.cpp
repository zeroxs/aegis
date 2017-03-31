//
// AegisAdmin.cpp
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

#include "AegisAdmin.h"
#include "Channel.h"
#include "Member.h"
#include "Guild.h"
#include "AegisBot.h"
#include <json.hpp>
#include <chrono>
#include "AegisBot.h"
#include <boost/algorithm/string.hpp>

AegisAdmin::AegisAdmin(AegisBot & bot, Guild & guild)
    : AegisModule(bot, guild)
{
    name = "admin";
}

void AegisAdmin::initialize()
{
    guild.addCommand("reload", std::bind(&AegisAdmin::reload, this, std::placeholders::_1));
    guild.addCommand("setgame", std::bind(&AegisAdmin::setGame, this, std::placeholders::_1));
    guild.addCommand("deletechannel", std::bind(&AegisAdmin::deletechannel, this, std::placeholders::_1));
    guild.addCommand("test", std::bind(&AegisAdmin::test, this, std::placeholders::_1));
    guild.addCommand("serverlist", std::bind(&AegisAdmin::serverList, this, std::placeholders::_1));
    guild.addCommand("leaveserver", std::bind(&AegisAdmin::leaveServer, this, std::placeholders::_1));
    guild.addCommand("serverinfo", std::bind(&AegisAdmin::serverInfo, this, std::placeholders::_1));
    guild.addCommand("disc", std::bind(&AegisAdmin::disc, this, std::placeholders::_1));
}

void AegisAdmin::remove()
{
    guild.removeCommand("reload");
    guild.removeCommand("setgame");
    guild.removeCommand("deletechannel");
    guild.removeCommand("test");
    guild.removeCommand("serverlist");
    guild.removeCommand("leaveserver");
    guild.removeCommand("serverinfo");
    guild.removeCommand("disc");
}

void AegisAdmin::disc(ABMessage & message)
{
    std::vector<string> tokens;
    boost::split(tokens, message.content, boost::is_any_of(" "));

    fmt::MemoryWriter w;
    int i = 0;

    for (auto & m : AegisBot::memberlist)
    {
        if (m.second->discriminator == std::stoi(tokens[1]))
        {
            ++i;
            w << m.second->getFullName() << "\n";
        }
    }
    if (i > 0)
        message.channel().sendMessage(w.str());
    else
        message.channel().sendMessage("None");
}

void AegisAdmin::serverInfo(ABMessage & message)
{
    std::vector<string> tokens;
    boost::split(tokens, message.content, boost::is_any_of(" "));

    uint64_t guildid = std::stoull(tokens[1]);

    if (tokens.size() >= 2)
    {
        try
        {
            Guild & guild = message.bot.getGuild(guildid);
            string title;

            title = fmt::format("Server: **{0}**", guild.name);

            uint64_t botcount = 0;
            for (auto & m : guild.memberlist)
                if (m.second.first->isbot)
                    botcount++;
            //TODO: may break until full member chunk is obtained on connect

            //TODO:2 this should be getMember
            Member & owner = guild.bot.createMember(guild.owner_id);
            fmt::MemoryWriter w;
            w   << "Owner name: " << owner.getFullName() << " [" << owner.getName(guild.id).value_or("") << "]\n"
                << "Owner id: " << guild.owner_id << "\n"
                << "Member count: " << guild.memberlist.size() << "\n"
                << "Channel count: " << guild.channellist.size() << "\n"
                << "Bot count: " << botcount << "\n"
                << "";

                
            json t = {
                { "title", title },
                { "description", w.str() },
                { "color", 10599460 }
            };
            message.channel().sendMessageEmbed(json(), t);
        }
        catch (std::domain_error & e)
        {
            message.channel().sendMessage(string(e.what()));
        }
    }
    else
    {
        message.channel().sendMessage("Invalid guild id.");
    }
}

void AegisAdmin::leaveServer(ABMessage & message)
{
    std::vector<string> tokens;
    boost::split(tokens, message.content, boost::is_any_of(" "));

    uint64_t guildid = std::stoull(tokens[1]);

    if (tokens.size() >= 2)
    {
        try
        {
            string guildname = message.bot.getGuild(guildid).name;
            message.channel().sendMessage(fmt::format("Leaving **{1}** [{0}]", tokens[1], message.bot.getGuild(guildid).name));
            message.bot.getGuild(guildid).leave(std::bind([](ABMessage & message, string id, string guildname, Channel * channel)
            {
                channel->sendMessage(fmt::format("Successfully left **{1}** [{0}]", id, guildname));
            }, std::placeholders::_1, tokens[1], guildname, &message.channel()));
        }
        catch (std::domain_error & e)
        {
            message.channel().sendMessage(string(e.what()));
        }
    }
    else
    {
        message.channel().sendMessage("Invalid guild id.");
    }
}

void AegisAdmin::test(ABMessage & message)
{
    message.channel().guild().bot.ws.close(message.channel().guild().bot.hdl, 1001, "");
}

void AegisAdmin::serverList(ABMessage & message)
{
    fmt::MemoryWriter w;
    for (auto & g : message.channel().guild().bot.guildlist)
    {
        w << "*" << g.second->name << "*  :  " << g.second->id << "\n";
    }


    json t = {
        { "title", "Server List" },
        { "description", w.str() },
        { "color", 10599460 }
    };
    message.channel().sendMessageEmbed(json(), t);
}

void AegisAdmin::deletechannel(ABMessage & message)
{
    for (auto & c : message.channel().guild().channellist)
    {
        if (c.second->type == ChannelType::VOICE)
        {
            c.second->deleteChannel();
        }
    }
}

void AegisAdmin::reload(ABMessage & message)
{
    if (message.member().id)
    {
        message.channel().sendMessage("Configs reloaded.");
    }
    else
    {
        message.channel().sendMessage("Not authorized.");
    }
}

void AegisAdmin::setGame(ABMessage & message)
{
    string gamestr = message.content.substr(message.channel().guild().prefix.size() + 8);
    AegisBot::io_service.post([game = std::move(gamestr), &bot = message.channel().guild().bot]()
    {
        /*json obj = {
            { "idle_since", nullptr },
            { "game", { "name", game }
            }
        };*/
        json obj;
        obj["op"] = 3;
        obj["d"]["idle_since"] = nullptr;
        //obj["d"]["since"] = epoch;
        //obj["d"]["afk"] = epoch;//clients
        //obj["d"]["status"] = epoch;//"online" "dnd" "idle" "invisible"

        obj["d"]["game"] = { { "name", game } };
        
        bot.wssend(obj.dump());
    });
}
