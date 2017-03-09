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
#include "rss.h"


//Example usage
class Bot
{
public:
    Bot() {}
    ~Bot() {}

    void echo(boost::shared_ptr<ABMessage> message)
    {
        if ((message->guild) && (message->channel) && (message->member))
        {
            //all 3 are set, so this is a channel message
            if (message->guild->id == 287048029524066334LL)
            {
                //control channel
                if (message->member->id == 171000788183678976LL)
                {
                    //me
                    std::cout << "Message callback triggered on channel[" << message->channel->name << "] from [" << message->member->name << "]" << std::endl;
                    message->channel->sendMessage(message->content.substr(message->cmd.size()+message->guild->prefix.size()));
                }
            }
        }
        else if (message->member)
        {

        }
    }

    void rates2(shared_ptr<ABMessage> message)
    {
        uint32_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        message->channel->sendMessage(Poco::format("Content: %s\nLimit: %u\nRemain: %u\nReset: %u\nEpoch: %u\nDiff: %u", message->content, message->channel->ratelimits.rateLimit()
        , message->channel->ratelimits.rateRemaining(), message->channel->ratelimits.rateReset(), epoch, message->channel->ratelimits.rateReset() - epoch));
    }

    void callback(ABMessage message)
    {
        //if (message)
            //sendMessage("data", );
    }


};

int main(int argc, char * argv[])
{
    uint64_t shardid = 0;

    if (argc == 1)
    {
        //error
        std::cout << "No shard id min/max passed (pass 0 for single instance)" << std::endl;
        return -1;
    }

    shardid = std::atoll(argv[1]);

    if (shardid != 0)
    {
        if (argc <= 2)
        {
            //error
            std::cout << "No shard id max passed." << std::endl;
            return -1;
        }
    }

    uint64_t maxshard;
    maxshard = std::atoll(argv[2]);

    if (maxshard <= shardid)
    {
        //error
        std::cout << "Shard id must be less than the max." << std::endl;
        return -1;
    }

    try
    {
        //create our Bot object and cache and configure the basic settings
        AegisBot & bot = AegisBot::CreateInstance();
 
        ABRedisCache cache(bot.io_service);
        cache.address = "127.0.0.1";
        cache.port = 6379;
        cache.password = "";
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

        //set up a single guild with the bot (official use will have this be dynamic and loaded from a DB)
        shared_ptr<Guild> myguild = bot.guildlist[287048029524066334LL] = boost::make_shared<Guild>(bot);

        myguild->prefix = "!";
        myguild->cmdlist["echo"] = std::bind(&Bot::echo, &bot, std::placeholders::_1);

        myguild->addCommand("timer", [&bot](shared_ptr<ABMessage> message)
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
        });
        myguild->cmdlist["rates2"] = std::bind(&Bot::rates2, &bot, std::placeholders::_1);
        myguild->addCommand("rates", [&bot](shared_ptr<ABMessage> message)
        {
            uint32_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            message->channel->sendMessage(Poco::format("Content: %s\nLimit: %u\nRemain: %u\nReset: %u\nEpoch: %u\nDiff: %u", message->content, message->channel->ratelimits.rateLimit()
                , message->channel->ratelimits.rateRemaining(), message->channel->ratelimits.rateReset(), epoch, message->channel->ratelimits.rateReset() - epoch));
        });
        myguild->addCommand("clean_channel", [&bot](shared_ptr<ABMessage> message)
        {
            message->channel->bulkDelete(bot.tempmessages);
        });
        myguild->addCommand("get_history", [&bot](shared_ptr<ABMessage> message)
        {
            message->channel->getMessages(message->channel->id, [&bot](shared_ptr<ABMessage> message)
            {
                json arr = json::parse(message->content);

                for (auto & m : arr)
                {
                    string entry = m["id"];
                    poco_trace_f1(*(bot.log), "Message entry: %s", entry);
                    bot.tempmessages.push_back(entry);
                }
            });
        });
        myguild->addCommand("reload", [&bot](shared_ptr<ABMessage> message)
        {
            bot.loadConfigs();
            message->channel->sendMessage("Configs reloaded.");
        });
        myguild->addCommand("info", [&bot](shared_ptr<ABMessage> message)
        {
            string stats;
            stats = Poco::format("```Memory usage: %[#2]fMB\nMax Memory: %[#2]fMB```", double(getCurrentRSS()) / (1024 * 1024), double(getPeakRSS()) / (1024 * 1024));
            message->channel->sendMessage(stats);
        });



        //find a better way to do this
        //ultimately, a master application would spin these bots up passing params
        //as the shard ids.
        json ret = json::parse(bot.call("/gateway/bot"));
        bot.gatewayurl = ret["url"];

        std::cout << "Recommended shard count: " << ret["shards"] << std::endl;
        std::cout << "Current set shard count: " << maxshard << std::endl;
        std::cout << "My shard id: " << shardid << std::endl;

        bot.initialize(shardid, maxshard);


        boost::asio::io_service::work work(bot.io_service);

        boost::asio::signal_set signals(bot.io_service, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code &error, int signal_number)
        {
            if (!error)
            {
                std::cerr << (signal_number==SIGINT?"SIGINT":"SIGTERM") << "received. Shutting down.\n";
                bot.io_service.stop();
            }
        });

        std::vector<std::thread> threadPool;
        for (size_t t = 0; t < std::thread::hardware_concurrency() * 2; t++)
        {
            threadPool.push_back(std::thread([&bot]() { bot.io_service.run(); }));
        }
        bot.run();
        for (std::thread& t : threadPool)
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
