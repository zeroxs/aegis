//
// Channel.cpp
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

#include "Channel.h"
#include "Guild.h"
#include "AegisBot.h"
#include "Member.h"


void Channel::getMessages(uint64_t messageid, ABMessageCallback callback)
{
    poco_trace(*(AegisBot::GetSingleton().log), "getMessages() goes through");

    boost::shared_ptr<ABMessage> message = boost::make_shared<ABMessage>();
    message->guild = belongs_to();
    message->channel = shared_from_this();
    message->endpoint = Poco::format("/channels/%Lu/messages", id);
    message->method = "GET";
    message->query = Poco::format("?before=%Lu&limit=100", messageid);
    if (callback)
        message->callback = callback;

    ratelimits.putMessage(message);
}

void Channel::sendMessage(string content, ABMessageCallback callback)
{
    poco_trace(*(AegisBot::GetSingleton().log), "sendMessage() goes through");
    json obj;
    if (belongs_to()->preventbotparse)
        obj["content"] = "\u200B" + content;
    else
        obj["content"] = content;

    boost::shared_ptr<ABMessage> message = boost::make_shared<ABMessage>();
    message->content = obj.dump();
    message->guild = belongs_to();
    message->channel = shared_from_this();
    message->endpoint = Poco::format("/channels/%Lu/messages", id);
    message->method = "POST";
    if (callback)
        message->callback = callback;

    ratelimits.putMessage(message);
}

void Channel::sendMessageEmbed(json content, json embed, ABMessageCallback callback /*= ABMessageCallback()*/)
{
    //if (!canSendMessages() || !canEmbed())
    //    return;
    poco_trace(*(AegisBot::GetSingleton().log), "sendMessageEmbed() goes through");
    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;

    poco_trace(*(AegisBot::GetSingleton().log), obj.dump());

    boost::shared_ptr<ABMessage> message = boost::make_shared<ABMessage>();
    message->content = obj.dump();
    message->guild = belongs_to();
    message->channel = shared_from_this();
    message->endpoint = Poco::format("/channels/%Lu/messages", id);
    message->method = "POST";
    if (callback)
        message->callback = callback;

    ratelimits.putMessage(message);
}

void Channel::bulkDelete(std::vector<string> messages, ABMessageCallback callback)
{
    poco_trace(*(AegisBot::GetSingleton().log), "bulkDelete() goes through");
    json arr(messages);
    json obj;
    obj["messages"] = arr;

    boost::shared_ptr<ABMessage> message = boost::make_shared<ABMessage>();
    message->content = obj.dump();
    message->guild = belongs_to();
    message->channel = shared_from_this();
    message->endpoint = Poco::format("/channels/%Lu/messages/bulk-delete", id);
    message->method = "POST";
    if (callback)
        message->callback = callback;

    ratelimits.putMessage(message);
}
