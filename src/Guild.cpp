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
    string avatar = author["avatar"];
    string username = author["username"];
    uint16_t discriminator = std::stoll(author["discriminator"].get<string>());

    uint64_t channel_id = std::stoll(obj["channel_id"].get<string>());
    uint64_t id = std::stoll(obj["id"].get<string>());
    uint64_t nonce = std::stoll(obj["nonce"].get<string>());

    string content = obj["content"];
    bool tts = obj["tts"];
    bool pinned = obj["pinned"];


    boost::char_separator<char> sep{ " " };
    boost::tokenizer<boost::char_separator<char>> tok{ content, sep };

    string cmd = (*tok.begin()).substr(1, (*tok.begin()).length() - 1);


    if (cmdlist.count(cmd))
    {
        //command exists - construct callback object and perform callback
        boost::shared_ptr<ABMessage> message = boost::make_shared<ABMessage>();
        message->content = content;
        message->channel = channellist[channel_id];
        message->member = AegisBot::GetSingleton().globalusers[userid];
        message->guild = this->shared_from_this();
        cmdlist[cmd](message);
    }
}

void Guild::addCommand(string command, std::function<void(boost::shared_ptr<ABMessage>)> callback)
{
    cmdlist[command] = callback;
}



