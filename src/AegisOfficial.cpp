//
// AegisOfficial.cpp
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

#include "AegisOfficial.h"
#include "Channel.h"
#include "Member.h"
#include "Guild.h"
#include "AegisBot.h"
#include <json.hpp>
#include <chrono>
#include "AegisBot.h"

AegisOfficial::AegisOfficial(AegisBot & bot, Guild & guild)
    : AegisModule(bot, guild)
{
    name = "default";
}

void AegisOfficial::initialize()
{
    guild.addCommand("createvoice", std::bind(&AegisOfficial::createVoice, this, std::placeholders::_1));
    guild.addCommand("info", std::bind(&AegisOfficial::info, this, std::placeholders::_1));
    guild.addCommand("clearchat", std::bind(&AegisOfficial::clearChat, this, std::placeholders::_1));
    guild.addCommand("clean", std::bind(&AegisOfficial::clean, this, std::placeholders::_1));
}

void AegisOfficial::remove()
{
    guild.removeCommand("createvoice");
    guild.removeCommand("info");
    guild.removeCommand("clearchat");
    guild.removeCommand("clean");
}

void AegisOfficial::clearChat(ABMessage & message)
{
    message.channel().getMessages(message.message_id, [](ABMessage & message)
    {
        try
        {
            uint32_t epoch = ((std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() - 14 * 24 * 60 * 60) - 1420070400000) << 22;
            std::vector<string> delmessages;
            for (auto & m : message.obj)
            {
                if (std::stoull(m["id"].get<string>()) > epoch)
                {
                    delmessages.push_back(m["id"].get<string>());
                }
            }
            message.channel().bulkDelete(delmessages);
        }
        catch (std::exception & e)
        {
            std::cout << "Error: " << e.what() << std::endl;
        }
    });
}

void AegisOfficial::clean(ABMessage & message)
{
    message.channel().getMessages(message.message_id, [](ABMessage & message)
    {
        message.channel().bulkDelete(message.obj);
    });
}

void AegisOfficial::createVoice(ABMessage & message)
{
    string name = getparams(message);
    if (!name.size())
    {
        message.channel().sendMessage("Not enough params");
        return;
    }
    json create;
    create = {
        { "name", name },
        { "type", "voice" },
        { "bitrate", 64000 }
    };
    message.channel().guild().createVoice(create, message.channel().guild().id, std::bind(&AegisOfficial::moveAfterCreate, this, std::placeholders::_1, message.member().id));
    message.channel().sendMessage(Poco::format("Channel created [%s]", name));
}

void AegisOfficial::moveAfterCreate(ABMessage & message, uint64_t member_id)
{
    json result = json::parse(message.content);
    json move;
    move = {
        { "channel_id", result["id"] }
    };
    message.channel().guild().modifyMember(move, message.channel().guild().id, member_id);
}

string AegisOfficial::getparams(ABMessage & message)
{
    if (message.content.size() > message.channel().guild().prefix.size() + message.cmd.size() + 1)
        return message.content.substr(message.channel().guild().prefix.size() + message.cmd.size() + 1);
    return "";
}

void AegisOfficial::info(ABMessage & message)
{
    uint64_t timenow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    uint64_t guild_count = AegisBot::guildlist.size();
    uint64_t member_count = AegisBot::globalusers.size();
    uint64_t channel_count = AegisBot::channellist.size();
    uint64_t channel_text_count = 0;
    uint64_t channel_voice_count = 0;
    {
        std::lock_guard<std::recursive_mutex> lock(AegisBot::m);
        for (auto & channel : AegisBot::channellist)
        {
            if (channel.second->type == ChannelType::TEXT)
                channel_text_count++;
            else
                channel_voice_count++;
        }
    }
    std::stringstream members;
    members << member_count << "\n0 Online\n0 Offline\nstuff";

    std::stringstream channels;
    channels << channel_count << " total\n" << channel_text_count << " text\n" << channel_voice_count << " voice";

    std::stringstream guilds;
    guilds << guild_count;

    message.content = "";
    string stats;
    stats = Poco::format("[Latest bot source](https://github.com/zeroxs/aegisbot)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)\n\nMemory usage: %.2fMB\nMax Memory: %.2fMB", double(AegisBot::getCurrentRSS()) / (1024 * 1024), double(AegisBot::getPeakRSS()) / (1024 * 1024));
    json t = {
        { "title", "AegisBot" },
        { "description", stats },
        { "color", 10599460 },
        { "fields",
        json::array(
    {
        { { "name", "Members" },{ "value", members.str() },{ "inline", true } },
        { { "name", "Channels" },{ "value", channels.str() },{ "inline", true } },
        { { "name", "Uptime test" },{ "value", uptime() },{ "inline", true } },
        { { "name", "Guilds" },{ "value", guilds.str() },{ "inline", true } }
    }
            )
        },
        { "footer",{ { "icon_url", "https://cdn.discordapp.com/attachments/288707540844412928/289572000391888906/cpp.png" },{ "text", "Made in c++ running aegisbot library" } } }
    };
    message.channel().sendMessageEmbed(json(), t);
}

string AegisOfficial::uptime()
{
    std::stringstream ss;
    std::chrono::steady_clock::time_point timenow = std::chrono::steady_clock::now();

    int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(timenow - AegisBot::starttime).count();

    uint32_t seconds = (ms / 1000) % 60;
    uint32_t minutes = (((ms / 1000) - seconds) / 60) % 60;
    uint32_t hours = (((((ms / 1000) - seconds) / 60) - minutes) / 60) % 24;
    uint32_t days = (((((((ms / 1000) - seconds) / 60) - minutes) / 60) - hours) / 24);

    if (days > 0)
        ss << days << "d ";
    if (hours > 0)
        ss << hours << "h ";
    if (minutes > 0)
        ss << minutes << "m ";
    if (seconds > 0)
        ss << seconds << "s";
    return ss.str();
}

