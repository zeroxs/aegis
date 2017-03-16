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

AegisAdmin::AegisAdmin(AegisBot & bot, Guild & guild)
    : AegisModule(bot, guild)
{
    name = "admin";
}

void AegisAdmin::initialize()
{
    guild.addCommand("reload", std::bind(&AegisAdmin::reload, this, std::placeholders::_1));
    guild.addCommand("rates", std::bind(&AegisAdmin::rates, this, std::placeholders::_1));
    guild.addCommand("setgame", std::bind(&AegisAdmin::setGame, this, std::placeholders::_1));
    guild.addCommand("deletechannel", std::bind(&AegisAdmin::deletechannel, this, std::placeholders::_1));
    guild.addCommand("test", std::bind(&AegisAdmin::test, this, std::placeholders::_1));
}

void AegisAdmin::remove()
{
    guild.removeCommand("reload");
    guild.removeCommand("rates");
    guild.removeCommand("setgame");
    guild.removeCommand("deletechannel");
    guild.removeCommand("test");
}

void AegisAdmin::reload(ABMessage & message)
{
    if (message.member().id)
    {
        bot.loadConfigs();
        message.channel().sendMessage("Configs reloaded.");
    }
    else
    {
        message.channel().sendMessage("Not authorized.");
    }
}

void AegisAdmin::rates(ABMessage & message)
{
    uint32_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    json t = {
        { "title", "AegisBot" },
        { "description", "" },
        { "color", 10599460 },
        { "fields",
        json::array(
    {
        { { "name", "Memory Usage" },{ "value", Poco::format("[Latest bot source](https://github.com/zeroxs/aegisbot)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)\n\nMemory usage: %.2fMB\nMax Memory: %.2fMB", double(AegisBot::getCurrentRSS()) / (1024 * 1024), double(AegisBot::getPeakRSS()) / (1024 * 1024)) } },
        { { "name", "Rates" },{ "value", Poco::format("Content: %s\nLimit: %u\nRemain: %u\nReset: %u\nEpoch: %u\nDiff: %u", message.content, message.channel().ratelimits.rateLimit()
        , message.channel().ratelimits.rateRemaining(), message.channel().ratelimits.rateReset(), epoch, message.channel().ratelimits.rateReset() - epoch) } }
    }
            )
        },
        { "footer",{ { "icon_url", "https://cdn.discordapp.com/attachments/288707540844412928/289572000391888906/cpp.png" },{ "text", "Made in c++ running aegisbot library" } } }
    };
    message.channel().sendMessageEmbed(json(), t);
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
