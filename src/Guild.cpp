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



Guild::Guild()
{
}

Guild::~Guild()
{
}

void Guild::processMessage(json obj)
{
    json author = obj["author"];
    uint64_t userid = std::stoll(author["id"].get<string>());

    //if this is my own message, ignore
    if (userid == AegisBot::GetSingleton().userId)
        return;

    string avatar = author["avatar"];
    string username = author["username"];
    uint16_t discriminator = std::stoll(author["discriminator"].get<string>());

    uint64_t channel_id = std::stoll(obj["channel_id"].get<string>());
    uint64_t id = std::stoll(obj["id"].get<string>());
    uint64_t nonce = std::stoll(obj["nonce"].get<string>());

    string content = obj["content"];
    bool tts = obj["tts"];
    bool pinned = obj["pinned"];

    //if message came from a bot, ignore
    if (AegisBot::GetSingleton().globalusers[userid]->isbot == true)
        return;

    if (userid == owner_id)
    {
        //guild owner is talking, do a check if this is the prefix set up command
        //as that needs to be set up first before the bot will function
        try
        {
#ifdef _DEBUG
            if (content.substr(0, 16) == "!?debugsetprefix")
            {
                string setprefix = content.substr(17);
#else
            if (content.substr(0, 11) == "!?setprefix")
            {
                string setprefix = content.substr(12);
#endif
                if (setprefix.size() == 0)
                    channellist[channel_id]->sendMessage("Invalid arguments. Prefix must have a length greater than 0");
                else
                {
                    prefix = setprefix;
                    channellist[channel_id]->sendMessage(Poco::format("Prefix successfully set to `%s`", setprefix));

                    //TODO: set this in a persistent DB to maintain across restarts
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

    if (content.substr(0, prefix.size()) != prefix)
        return;

    boost::char_separator<char> sep{ " " };
    boost::tokenizer<boost::char_separator<char>> tok{ content, sep };

    string cmd = (*tok.begin()).substr(prefix.size());


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
            if (clientlist[userid].second < cmdlist[cmd].first.level)
            {
                if (!silentperms)
                {
                    channellist[channel_id]->sendMessage("You do not have access to that command.");
                    return;
                }
            }
        }


        boost::shared_ptr<ABMessage> message = boost::make_shared<ABMessage>();
        message->content = content;
        message->channel = channellist[channel_id];
        message->member = AegisBot::GetSingleton().globalusers[userid];
        message->guild = this->shared_from_this();
        message->message_id = id;
        message->cmd = cmd;
        cmdlist[cmd].second(message);
    }
}

void Guild::addCommand(string command, ABMessageCallback callback)
{
    cmdlist[command] = ABCallbackPair(ABCallbackOptions(), callback);
}

void Guild::addCommand(string command, ABCallbackPair callback)
{
    cmdlist[command] = callback;
}




