//
// Guild.cpp
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

#include "Guild.h"
#include "AegisBot.h"
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>


Guild::Guild(AegisBot & bot, uint64_t id)
    : bot(bot)
    , id(id)
{

}

Guild::~Guild()
{

}

void Guild::processMessage(json obj)
{
    json author = obj["author"];
    uint64_t userid = std::stoull(author["id"].get<std::string>());

    //if this is my own message, ignore
#ifndef SELFBOT
    if (userid == AegisBot::userId)
        return;
#else
    if (userid != AegisBot::userId)
        return;
#endif

    std::string avatar = author["avatar"].is_string()?author["avatar"]:"";
    std::string username = author["username"];
    //uint16_t discriminator = std::stoi(author["discriminator"].get<string>());

    uint64_t channel_id = std::stoull(obj["channel_id"].get<std::string>());
    uint64_t id = std::stoull(obj["id"].get<std::string>());
    //uint64_t nonce = obj["nonce"].is_null()?0:std::stoull(obj["nonce"].get<string>());

    std::string content = obj["content"];
    //bool tts = obj["tts"];
    //bool pinned = obj["pinned"];

    //if message came from a bot, ignore
//     if (!AegisBot::memberlist.count(userid) || AegisBot::memberlist[userid]->isbot == true)
//         return;



    if (userid == ROOTADMIN)
    {

        if (content.substr(0, AegisBot::mention.size()) == AegisBot::mention)
        {

            boost::char_separator<char> sep{ " " };
            boost::tokenizer<boost::char_separator<char>> tok{ content, sep };

            auto token = tok.begin();

            ++token;

            if (token == tok.end()) return;

            std::string cmd = *(token++);

#ifdef _DEBUG
            if (cmd == "setprefix")
            {
                if (token == tok.end()) return;
                std::string setprefix = *(token++);
                if (setprefix.size() == 0)
                    channellist[channel_id]->sendMessage("Invalid arguments. Prefix must have a length greater than 0");
                else
                {
                    prefix = setprefix;
                    channellist[channel_id]->sendMessage(fmt::format("Prefix successfully set to `{0}`", setprefix));

                    //TODO: set this in a persistent DB to maintain across restarts
                }
                return;
            }
#endif

            if (cmd == "exit")
            {
                //TODO: add some core bot management
                bot.getChannel(1).sendMessage(fmt::format("Bot shutdown g[{0}] c[{1}] u[{2}]", id, channel_id, userid));
                channellist[channel_id]->sendMessage("Bot shutting down.");
                std::this_thread::sleep_for(std::chrono::seconds(2));
                bot.ws.close(bot.hdl, 1000, "");
                bot.io_service.stop();
                bot.isrunning = false;
                bot.active = false;
                return;
            }
        }
    }


    if ((userid == owner_id) || (userid == ROOTADMIN))//for support
    {
        //guild owner is talking, do a check if this is the prefix set up command
        //as that needs to be set up first before the bot will function
        try
        {
            if (content.substr(0, AegisBot::mention.size()) == AegisBot::mention)
            {

                boost::char_separator<char> sep{ " " };
                boost::tokenizer<boost::char_separator<char>> tok{ content, sep };

                auto token = tok.begin();

                token++;
                std::string cmd = *(token++);

                if (cmd == "setprefix")
                {
                    std::string setprefix = *(token++);
                    if (setprefix.size() == 0)
                        channellist[channel_id]->sendMessage("Invalid arguments. Prefix must have a length greater than 0");
                    else
                    {
                        prefix = setprefix;
                        channellist[channel_id]->sendMessage(fmt::format("Prefix successfully set to `{0}`", setprefix));

                        //TODO: set this in a persistent DB to maintain across restarts
                    }
                }
                else if (cmd == "help")
                {
                    channellist[channel_id]->sendMessage("This bot is in development. If you'd like more information on it, please join the discord server https://discord.gg/w7Y3Bb8");
                    return;
                }
                else if (cmd == "wl")
                {
                    try
                    {
                        uint64_t channel = stoull(*(token++));
                        activeChannels.push_back(channel);
                        channellist[channel_id]->sendMessage(fmt::format("Channel [<#{0}>] added to whitelist", channel));
                    }
                    catch (std::exception&e)
                    {
                        activeChannels.push_back(channel_id);
                        channellist[channel_id]->sendMessage(fmt::format("Channel [<#{0}>] added to whitelist", channel_id));
                    }
                    return;
                }
                else if (cmd == "bl")
                {
                    try
                    {
                        uint64_t channel = stoull(*(token++));
                        auto it = std::find(activeChannels.begin(), activeChannels.end(), channel);
                        if (it != activeChannels.end())
                        {
                            activeChannels.erase(it);
                            channellist[channel_id]->sendMessage(fmt::format("Channel [<#{0}>] removed from whitelist", channel));
                            return;
                        }
                    }
                    catch (std::exception&e)
                    {
                        auto it = std::find(activeChannels.begin(), activeChannels.end(), channel_id);
                        if (it != activeChannels.end())
                        {
                            activeChannels.erase(it);
                            channellist[channel_id]->sendMessage(fmt::format("Channel [<#{0}>] removed from whitelist", channel_id));
                            return;
                        }
                    }
                    return;
                }
                else if (cmd == "commands")
                {
                    if (std::find(activeChannels.begin(), activeChannels.end(), channel_id) == activeChannels.end())
                        return;
                    std::stringstream ss;
                    for (auto & c : cmdlist)
                    {
                        ss << " " << c.first;
                    }
                    ss << " ";
                    channellist[channel_id]->sendMessage(fmt::format("Command list: `{0}`.", ss.str()));
                }
                return;
            }
        }
        catch (std::exception&e)
        {
            channellist[channel_id]->sendMessage("Invalid arguments. Prefix must have a length greater than 0");
            return;
        }
    }

    if (std::find(activeChannels.begin(), activeChannels.end(), channel_id) == activeChannels.end())
        return;

    if (content.size() == 1 || content.size() - prefix.size() == 0 || content.substr(0, prefix.size()) != prefix)
        return;
    
    boost::char_separator<char> sep{ " " };
    boost::tokenizer<boost::char_separator<char>> tok{ content, sep };

    //TODO: 
    //boost::tokenizer<boost::char_separator<char>> tok{ content.substr(prefix.size()), sep };
    //vs
    //string cmd = (*(token++)).substr(prefix.size());

    if (tok.begin() == tok.end())
        return;

    auto token = tok.begin();
    std::string cmd = (*(token++)).substr(prefix.size());

    //check if attachment exists
    if (obj.count("attachments") > 0)
    {
/*
        std::cout << "Attachments check true" << std::endl;
        for (auto & attach : obj["attachments"])
        {
            std::cout << "Attachment found" << obj["url"] << std::endl;
            if (attachmenthandler.second && attachmenthandler.first.enabled)
            {
                ABMessage message(channellist[channel_id], AegisBot::memberlist[userid]);
                message.message_id = id;
                message.obj = obj;
                attachmenthandler.second(message);
                return;
            }
        }*/
    }

    if (cmdlist.count(cmd))
    {
        //guild owners bypass all restrictions
        if (userid != owner_id)
        {
            //command exists - construct callback object and perform callback
            //but first check if it's enabled
            if (!cmdlist[cmd].first.enabled)
                return;

            //now check access levels
            //a user that does not exist in the access list has a default permission level of 0
            //commands have a default setting of 1 preventing anyone but guild owner from initially
            //running any commands until permissions are set
            if (memberlist[userid].second < cmdlist[cmd].first.level)
            {
                if (!silentperms)
                {
                    channellist[channel_id]->sendMessage("You do not have access to that command.");
                    return;
                }
            }
        }


        ABMessage message(channellist[channel_id], AegisBot::memberlist[userid]);
        message.content = content;
        message.message_id = id;
        message.cmd = cmd;
        cmdlist[cmd].second(message);
        return;
    }
}

void Guild::addCommand(std::string command, ABMessageCallback callback)
{
    cmdlist[command] = ABCallbackPair(ABCallbackOptions(), callback);
}

void Guild::addCommand(std::string command, ABCallbackPair callback)
{
    cmdlist[command] = callback;
}

void Guild::removeCommand(std::string command)
{
    if (cmdlist.count(command) > 0)
    {
        cmdlist.erase(command);
    }
}

//TODO: add support for multiple maybe?
void Guild::addAttachmentHandler(ABMessageCallback callback)
{
    attachmenthandler = ABCallbackPair(ABCallbackOptions(), callback);
}

void Guild::addAttachmentHandler(ABCallbackPair callback)
{
    attachmenthandler = callback;
}

void Guild::removeAttachmentHandler()
{
    attachmenthandler = ABCallbackPair();
}

void Guild::modifyMember(json content, uint64_t guildid, uint64_t memberid, ABMessageCallback callback)
{
    //TODO: also needs to check perm for connecting to channel
    if (content.count("channel_id") && !canVoiceMove())
    {
        if (silentperms)
            return;
        else
            throw no_permission("MOVE_MEMBERS");
    }
    if (content.count("nick") && !canManageNames())
    {
        if (silentperms)
            return;
        else
            throw no_permission("MANAGE_NICKNAMES");
    }
    if (content.count("roles") && !canManageRoles())
    {
        if (silentperms)
            return;
        else
            throw no_permission("MANAGE_ROLES");
    }
    if (content.count("mute") && !canVoiceMute())
    {
        if (silentperms)
            return;
        else
            throw no_permission("MUTE_MEMBERS");
    }
    if (content.count("deaf") && !canVoiceDeafen())
    {
        if (silentperms)
            return;
        else
            throw no_permission("DEAFEN_MEMBERS");
    }


    ABMessage message(this);
    message.content = content.dump();
    message.endpoint = fmt::format("/guilds/%Lu/members/{0}", guildid, memberid);
    message.method = "PATCH";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Guild::createVoice(json content, uint64_t guildid, ABMessageCallback callback)
{
    if (!canManageGuild())
    {
        if (silentperms)
            return;
        else
            throw no_permission("MANAGE_SERVER");
    }

    ABMessage message(this);
    message.content = content.dump();
    message.endpoint = fmt::format("/guilds/{0}/channels", guildid);
    message.method = "POST";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Guild::leave(ABMessageCallback callback)
{
    ABMessage message(this);
    message.endpoint = fmt::format("/users/@me/guilds/{0}", id);
    message.method = "DELETE";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}
