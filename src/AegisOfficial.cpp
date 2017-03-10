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


AegisOfficial::AegisOfficial()
{

}

AegisOfficial::~AegisOfficial()
{

}

void AegisOfficial::initialize()
{
    AegisBot & bot = AegisBot::GetSingleton();

    bot.addCommand("createvoice", std::bind(&AegisOfficial::createVoice, this, std::placeholders::_1));

}

void AegisOfficial::createVoice(shared_ptr<ABMessage> message)
{
    string name = getparams(message);
    if (!name.size())
    {
        message->channel->sendMessage("Not enough params");
        return;
    }
    json create;
    create = {
        { "name", name },
        { "type", "voice" },
        { "bitrate", 64000 }
    };
    message->guild->createVoice(create, message->guild->id, std::bind(&AegisOfficial::moveAfterCreate, this, std::placeholders::_1, message->member->id));
    message->channel->sendMessage(Poco::format("Channel created [%s]", name));
}

void AegisOfficial::moveAfterCreate(shared_ptr<ABMessage> message, uint64_t member_id)
{
    json result = json::parse(message->content);
    json move;
    move = {
        { "channel_id", result["id"] }
    };
    message->guild->modifyMember(move, message->guild->id, member_id);
}

string AegisOfficial::getparams(shared_ptr<ABMessage> message)
{
    if (message->content.size() > message->guild->prefix.size() + message->cmd.size() + 1)
        return message->content.substr(message->guild->prefix.size() + message->cmd.size() + 1);
    return "";
}
