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
#include "ExampleBot.h"
#include "AuctionBot.h"
#include "AegisOfficial.h"

uint64_t maxshard = 0;
uint64_t shardid = 0;

bool parsecli(int argc, char * argv[]);

void this_is_a_global_function(boost::shared_ptr<ABMessage> message)
{
    message->channel->sendMessage("return from this_is_a_global_function()");
}

int main(int argc, char * argv[])
{
    if (!parsecli(argc, argv))
        return -1;

    try
    {
        //create our Bot object and cache and configure the basic settings
        AegisBot & bot = AegisBot::CreateInstance();

#ifdef USE_REDIS
        ABRedisCache cache(bot.io_service);
        cache.address = "127.0.0.1";
        cache.port = 6379;
        cache.password = "";
#endif
        if (!cache.initialize())
        {
            //error with cache
            return -1;
        }
        bot.setup_cache(&cache);

        bot.loadConfigs();

        //this is temporary
#ifdef USE_MEMORY
        string token = "yourtokenhere";
#endif


        //register some callbacks

        //we can register a callback to a lambda function, or our own class, or even global space functions
        //note, these are global functions and will exist in all guilds the bot is connected to.

        //create our class that contains some functions (optional)
        ExampleBot ourfunctionclass;

        //can also load multiple classes worth of functions as callbacks
        //this is an example of an auctioneer bot I wrote to test some of the functionality and flexibility of the library
        AuctionBot auctionbot;

        //initialize defaults and add all the functions to a specific guild
        auctionbot.initialize();



        //This extends the bot and provides official commands
        AegisOfficial official;
        official.initialize();



        bot.addCommand("this_is_a_global_command", std::bind(&this_is_a_global_function, std::placeholders::_1));
        bot.addCommand("this_is_a_class_command", std::bind(&ExampleBot::this_is_a_class_function, &ourfunctionclass, std::placeholders::_1));

        //you can also set the command directly instead of calling addCommand()
        bot.defaultcmdlist["this_is_a_lambda_command"] = ABCallbackPair(ABCallbackOptions(), [](shared_ptr<ABMessage> message)
        {
            message->channel->sendMessage("return from this_is_a_lambda_function()");
        });
        bot.defaultcmdlist["echo"] = ABCallbackPair(ABCallbackOptions(), std::bind(&ExampleBot::echo, &ourfunctionclass, std::placeholders::_1));


        //add unique commands to a specific guild. no other guilds can access these
        auto myguild = AegisBot::CreateGuild(287048029524066334LL);
        myguild->addCommand("custom_command", [&bot](shared_ptr<ABMessage> message)
        {
            message->channel->sendMessage("unique command for this guild.");
        });


        //example usage of callbacks within a callback
        //this deletes the last 100 messages in the channel this is called from
        bot.addCommand("delete_history", [&bot](shared_ptr<ABMessage> message)
        {
            message->channel->getMessages(message->message_id, [&bot](shared_ptr<ABMessage> message)
            {
                json arr = json::parse(message->content);

                std::vector<string> tempmessages;

                for (auto & m : arr)
                {
                    string entry = m["id"];
                    poco_trace_f1(*(bot.log), "Message entry: %s", entry);
                    tempmessages.push_back(entry);
                }

                message->channel->bulkDelete(tempmessages);
            });
        });


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

        bot.addCommand("rates", [&bot](shared_ptr<ABMessage> message)
        {
            std::cout << "Rates Command()" << std::endl;
            uint32_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            message->channel->sendMessage(Poco::format("```Previous counters:\nContent: %s\nLimit: %u\nRemain: %u\nReset: %u\nEpoch: %u\nDiff: %u```", message->content, message->channel->ratelimits.rateLimit()
                , message->channel->ratelimits.rateRemaining(), message->channel->ratelimits.rateReset(), epoch, message->channel->ratelimits.rateReset() - epoch));
        });

        bot.addCommand("reload", [&bot](shared_ptr<ABMessage> message)
        {
            bot.loadConfigs();
            message->channel->sendMessage("Configs reloaded.");
        });

        bot.addCommand("info", std::bind(&AegisBot::info_command, &bot, std::placeholders::_1));





        //////////////////////////////////////////////////////////////////////////
        //find a better way to do this
        //ultimately, a master application would spin these bots up passing params
        //as the shard ids. alternatively, could make this bot do some calls to
        //load more bot instances at this point and have those ignore this part
        string res;
        bot.call("/gateway/bot", &res, nullptr, "GET", "");
        json ret = json::parse(res);
        bot.gatewayurl = ret["url"];

        std::cout << "Recommended shard count: " << ret["shards"] << std::endl;
        std::cout << "Current set shard count: " << maxshard << std::endl;
        std::cout << "My shard id: " << shardid << std::endl;

        bot.initialize(shardid, maxshard);
        bot.run();

        for (std::thread& t : bot.threadPool)
        {
            t.join();
        }
    }
    catch (std::exception & e)
    {
        std::cout << e.what() << std::endl;
        return -1;
    }
    return 0;
}

bool parsecli(int argc, char * argv[])
{
    if (argc == 1)
    {
        //error
        std::cout << "No shard id min/max passed (pass 0 for single instance)" << std::endl;
        return false;
    }

    shardid = std::atoll(argv[1]);

    if (shardid != 0)
    {
        if (argc <= 2)
        {
            //error
            std::cout << "No shard id max passed." << std::endl;
            return false;
        }
    }

    maxshard = std::atoll(argv[2]);

    if (maxshard <= shardid)
    {
        //error
        std::cout << "Shard id must be less than the max." << std::endl;
        return false;
    }
    return true;
}
