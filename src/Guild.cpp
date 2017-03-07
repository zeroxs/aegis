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
#include <boost/algorithm/string.hpp>



Guild::Guild(Bot & b)
    : bot(b)
{
}

Guild::~Guild()
{
}

void Guild::processMessage(json obj)
{
    json author = obj["author"];
    uint64_t userid = author["id"];
    string avatar = author["avatar"];
    string username = author["username"];
    uint16_t discriminator = author["discriminator"];

    uint64_t channel_id = obj["channel_id"];
    uint64_t id = obj["id"];
    uint64_t nonce = obj["nonce"];

    string content = obj["content"];
    bool tts = obj["tts"];
    bool pinned = obj["pinned"];


    std::vector<string> tokens;
    boost::split(tokens, content, boost::is_any_of(" "));

    string cmd = tokens[0].substr(1, tokens[0].length() - 1);

    if (cmd == "echo")
    {
        echo(content, channel_id);
    }
    else if (cmd == "timer")
    {
        int64_t time = Var(tokens[1]).convert<int64_t>();
        if (time > 1000 * 60 * 60)//1 hour max timer
        {
            bot.sendMessage(Poco::format("Timer too large `%Ld`", time), channel_id);
            return;
        }
        bot.sendMessage(Poco::format("Timer started `%Ld`", time), channel_id);
        std::thread t([&]() {
            bot.io_service.post([&, time]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(time));
                bot.sendMessage(Poco::format("Response to `%s`", content), channel_id);
            });
        });
    }
    else if (cmd == "clean_channel")
    {
        bot.bulkDelete(channel_id, bot.tempmessages);
    }
    else if (cmd == "get_history")
    {
        bot.getMessages(channel_id, id);
    }




}

void Guild::echo(string content, uint64_t channel_id)
{
    bot.sendMessage(Poco::format("Response to `%s`", content), channel_id);
}
