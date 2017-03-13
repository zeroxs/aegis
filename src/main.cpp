//
// main.cpp
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

#include "AegisBot.h"
#include "ABRedisCache.h"
#include <boost/lexical_cast.hpp>
#include <json.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/tuple/tuple.hpp>
#include "ExampleBot.h"
#include "AuctionBot.h"
#include "AegisOfficial.h"

//////////////////////////////////////////////////////////////////////////
//
void this_is_a_global_function(shared_ptr<ABMessage> message)
{
    message->channel->sendMessage("return from this_is_a_global_function()");
}

int main(int argc, char * argv[])
{
    try
    {
        boost::asio::io_service::work work(AegisBot::io_service);
        AegisBot::pFC = new FormattingChannel(new PatternFormatter("%p:%T %t"));
        AegisBot::pFC->setChannel(new ConsoleChannel);
        AegisBot::pFC->open();

        File f("log/");
        if (!f.exists())
        {
            f.createDirectory();
        }
        else if (f.isFile())
        {
            throw std::runtime_error("Error creating log directory!");
        }

        AegisBot::pFCf = new FormattingChannel(new PatternFormatter("%Y-%m-%d %H:%M:%S.%c | %s:%q:%t"));
        AegisBot::pFCf->setChannel(new FileChannel("log/console.log"));
        AegisBot::pFCf->setProperty("rotation", "daily");
        AegisBot::pFCf->setProperty("times", "local");
        AegisBot::pFCf->open();
        AegisBot::logf = &Poco::Logger::create("fileLogger", AegisBot::pFCf, Message::PRIO_TRACE);
        AegisBot::log = &Poco::Logger::create("consoleLogger", AegisBot::pFC, Message::PRIO_TRACE);


#ifdef USE_REDIS
        ABRedisCache cache(AegisBot::io_service);
        cache.address = "127.0.0.1";
        cache.port = 6379;
        cache.password = "";
#endif
        if (!cache.initialize())
        {
            //error with cache
            return -1;
        }
        AegisBot::setupCache(&cache);

        AegisBot::loadConfigs();

        //create our Bot object and cache and configure the basic settings
        AegisBot::startShards();

        AegisBot & bot = *AegisBot::bots[0];


        //this is temporary
#ifdef USE_MEMORY
        string token = "yourtokenhere";
#endif
#ifdef SELFBOT
        string token = "yourtokenhere";
#endif


        //add unique commands to a specific guild. no other guilds can access these
        auto myguild = bot.CreateGuild(287048029524066334LL);
        myguild->addCommand("custom_command", [&bot](shared_ptr<ABMessage> message)
        {
            message->channel->sendMessage("unique command for this guild.");
        });

        //Add the default module along with all the default commands
        myguild->addModule("default");





/*
        bot.addCommand("timer", [&bot](shared_ptr<ABMessage> message)
        {
            uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

            std::vector<string> tokens;
            boost::split(tokens, message->content, boost::is_any_of(" "));

            if (tokens.size() < 3)
            {
                //not enough tokens
                message->channel->sendMessage(Poco::format("Error parsing. Format is %s%s time-in-seconds message", message->guild->prefix, message->cmd));
                return;
            }

            int64_t time = std::stoll(tokens[1]);
            if (time > 1000 * 60 * 60)//1 hour max timer
            {
                //fix this mess, have Channel class have a sendMessage function instead of guild
                message->channel->sendMessage(Poco::format("Timer too large `%Ld`", time));
                return;
            }
            message->channel->sendMessage(Poco::format("Timer started `%Ld`", time));
            //this WILL instantly fire and possibly crash since t loses scope. leaving it here just because.
            //add a timers vector to the bot that tracks users, channels, reasons, and times and have main loop
            //process it instead
            std::thread t([&]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(time));
                bot.io_service.post([message, time]()
                {
                    message->channel->sendMessage(Poco::format("Response to `%s`", message->content));
                });
            });
        });*/


        AegisBot::threads();
        AegisBot::workthread.join();
        Poco::Logger::shutdown();
        AegisBot::pFCf->close();
        AegisBot::pFC->close();
    }
    catch (std::exception & e)
    {
        std::cout << e.what() << std::endl;
        return -1;
    }
    return 0;
}
