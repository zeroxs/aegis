//
// AegisBot.cpp
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
#include "Guild.h"
#include <mutex>

#ifdef USE_REDIS
#include "ABRedisCache.h"
#endif

FormattingChannel * AegisBot::pFC;
FormattingChannel * AegisBot::pFCf;
Logger * AegisBot::log;
Logger * AegisBot::logf;
string AegisBot::gatewayurl;
bool AegisBot::isrunning;
bool AegisBot::active;
std::recursive_mutex AegisBot::m;
std::vector<std::thread> AegisBot::threadPool;
std::thread AegisBot::workthread;
string AegisBot::token;
std::chrono::steady_clock::time_point AegisBot::starttime;
ABCache * AegisBot::cache;
string AegisBot::username;
bool AegisBot::rate_global;
uint16_t AegisBot::discriminator;
string AegisBot::avatar;
uint64_t AegisBot::userId;
bool AegisBot::mfa_enabled;
std::map<uint64_t, AegisBot::PrivateChat> AegisBot::private_channels;
std::map<uint64_t, Channel*> AegisBot::channellist;
std::map<uint64_t, Member*> AegisBot::globalusers;
std::map<uint64_t, Guild*> AegisBot::guildlist;
std::vector<std::unique_ptr<AegisBot>> AegisBot::shards;
boost::asio::io_service AegisBot::io_service;
uint16_t AegisBot::shardidmax;
string AegisBot::mention;

AegisBot::AegisBot()
    : keepalive_timer_(io_service)
{

}

AegisBot::~AegisBot()
{

}

void AegisBot::cleanup()
{
    for (auto g : guildlist)
    {
        delete g.second;
    }
    for (auto g : globalusers)
    {
        delete g.second;
    }
    for (auto g : channellist)
    {
        delete g.second;
    }
}

Guild & AegisBot::createGuild(uint64_t id)
{
    if (AegisBot::guildlist.count(id))
        return *AegisBot::guildlist[id];
    //AegisBot::guildlist[id] = Guild(*this, id);
    AegisBot::guildlist.insert(std::pair<uint64_t, Guild*>(id, new Guild(*this, id)));
    AegisBot::guildlist[id]->id = id;
    return *AegisBot::guildlist[id];
}

Member & AegisBot::createMember(uint64_t id)
{
    if (AegisBot::globalusers.count(id))
        return *AegisBot::globalusers[id];
    //AegisBot::globalusers[id] = Member();
    AegisBot::globalusers.insert(std::pair<uint64_t, Member*>(id, new Member()));
    AegisBot::globalusers[id]->id = id;
    return *AegisBot::globalusers[id];
}

Channel & AegisBot::createChannel(uint64_t id, uint64_t guildid)
{
    if (AegisBot::channellist.count(id))
        return *AegisBot::channellist[id];
    //AegisBot::channellist[id] = Channel(CreateGuild(guildid));
    AegisBot::channellist.insert(std::pair<uint64_t, Channel*>(id, new Channel(&createGuild(guildid))));
    AegisBot::channellist[id]->id = id;
    return *AegisBot::channellist[id];
}

Guild & AegisBot::getGuild(uint64_t id)
{
    if (AegisBot::guildlist.count(id))
        return *AegisBot::guildlist[id];
}

Member & AegisBot::getMember(uint64_t id)
{
    if (AegisBot::globalusers.count(id))
        return *AegisBot::globalusers[id];
}

Channel & AegisBot::getChannel(uint64_t id)
{
    if (AegisBot::channellist.count(id))
        return *AegisBot::channellist[id];
}

void AegisBot::setupCache(ABCache * in)
{
    cache = in;
}

void AegisBot::loadConfigs()
{
    //TODO: might need to add a mutex here to prevent actions from running while configs reload
#ifdef USE_REDIS
    int32_t level = boost::lexical_cast<int32_t>(cache->get("config:loglevel"));
#ifdef SELFBOT
    token = cache->get("config:token2");
#else
    token = cache->get("config:token");
#endif
    logf->setLevel(level);
    log->setLevel(level);
#endif
}

bool AegisBot::initialize(uint64_t shardid)
{
    this->shardid = shardid;
    

    if (token == "")
    {
        std::cout << "Bot token is not set." << std::endl;
        return false;
    }

    //ws.set_access_channels(websocketpp::log::alevel::all);
    //ws.clear_access_channels(websocketpp::log::alevel::frame_payload);
    ws.clear_access_channels(websocketpp::log::alevel::all);

    ws.set_tls_init_handler([](websocketpp::connection_hdl)
    {
        return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);
    });

    ws.init_asio(&io_service);

    ws.set_message_handler(std::bind(&AegisBot::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    ws.set_open_handler(std::bind(&AegisBot::onConnect, this, std::placeholders::_1));
    ws.set_close_handler(std::bind(&AegisBot::onClose, this, std::placeholders::_1));

    websocketpp::lib::error_code ec;
    std::cout << "Connecting to gateway at " << gatewayurl << "\n";
    connection = ws.get_connection(gatewayurl + "/?encoding=json&v=6", ec);
    if (ec)
    {
        std::cout << "Connection failed: " << ec.message() << std::endl;
        return false;
    }
    else
    {
        ws.connect(connection);
        return true;
    }
}

void AegisBot::startShards()
{
    starttime = std::chrono::steady_clock::now();
    shardidmax = 0;
    isrunning = true;
    active = false;
    mfa_enabled = false;
    rate_global = false;
    string res;
    bool success;

    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code &error, int signal_number)
    {
        if (!error)
        {
            std::cerr << (signal_number == SIGINT ? "SIGINT" : "SIGTERM") << "received. Shutting down.\n";
            io_service.stop();
            isrunning = false;
        }
    });

#ifdef SELFBOT
    boost::tie(success, res) = call("/gateway");
#else
    boost::tie(success, res) = call("/gateway/bot");
#endif
    json ret = json::parse(res);
    gatewayurl = ret["url"];

#ifndef SELFBOT
    std::cout << "Shard count: " << ret["shards"] << std::endl;
    shardidmax = ret["shards"];
#endif

    for (int i = 0; i < AegisBot::shardidmax; ++i)
    {
        std::cout << "Shard id: " << i << std::endl;
        AegisBot::shards.push_back(std::make_unique<AegisBot>());
        AegisBot::shards[i]->initialize(i);
        AegisBot::threadPool.push_back(std::thread([=]() { AegisBot::shards[i]->run(); }));
    }
}

void AegisBot::threads()
{
    for (size_t t = 0; t < std::thread::hardware_concurrency() * 2; t++)
        threadPool.push_back(std::thread([&]() { io_service.run(); }));
    for (auto & b : AegisBot::shards)
        AegisBot::threadPool.push_back(std::thread([&]() { b->run(); }));
    for (std::thread& t : AegisBot::threadPool)
        t.join();
}

void AegisBot::connectWS()
{
    //make a portable way to do this even though I like the atomic-ness of redis scripts for obtaining inter-process locks
#ifdef USE_REDIS
/*
    while (true)
    {
        uint64_t epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        //string obtainlock = Poco::format("\"local time = redis.call('get', KEYS[1]) if (not time) then redis.call('set', KEYS[1], ARGV[1]) return 0 end if (time < ARGV[1]) then return 0 else redis.call('set', KEYS[1], ARGV[1]) return 1 end\" 1 config:wslock %Lu", epoch);
        //string res = static_cast<ABRedisCache*>(cache)->eval(obtainlock);
        if (res == "1")
            break;
        else
        {
            log->warning("Websocket lock held. Waiting to reconnect.");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (!isrunning)
                return;
        }
    }
*/

#endif

    websocketpp::lib::error_code ec;
    connection = ws.get_connection(gatewayurl + "/?encoding=json&v=6", ec);
    ws.connect(connection);
}

void AegisBot::onClose(websocketpp::connection_hdl hdl)
{
    keepalive_timer_.cancel();
    poco_error(*log, "Connection closed.");
    poco_information(*log, "Reconnecting.");
    if (isrunning)
        connectWS();
}

void AegisBot::processReady(json & d)
{
    std::lock_guard<std::recursive_mutex> lock(m);
    json guilds = d["guilds"];
    for (auto & guildobj : guilds)
    {
        uint64_t id = std::stoull(guildobj["id"].get<string>());

        bool unavailable = false;
        if (guildobj.count("unavailable"))
            unavailable = guildobj["unavailable"];

        Guild & guild = createGuild(id);
        poco_trace_f1(*log, "Guild created: %Lu", id);
        guildlist[id]->unavailable = unavailable;
        if (!unavailable)
        {
            loadGuild(guildobj);
        }
    }

    json presences = d["presences"];
    for (auto & presenceobj : presences)
    {
        //TODO
    }

    private_channels.clear();
    json pchannels = d["private_channels"];
    for (auto & channel : pchannels)
    {
        uint64_t channel_id = std::stoll(channel["id"].get<string>());
        uint64_t last_message_id = channel["last_message_id"].is_null()?0:std::stoll(channel["last_message_id"].get<string>());
        int32_t channelType = channel["type"];
        json recipients = channel["recipients"];
      
        PrivateChat & privateChat = private_channels[channel_id];//test
        poco_trace_f1(*log, "Private Channel created: %Lu", channel_id);
        
        for (auto & recipient : recipients)
        {
            string recipientAvatar = recipient["avatar"].is_null()?"":recipient["avatar"];
            uint16_t recipientDiscriminator = std::stoll(recipient["discriminator"].get<string>());
            string recipientName = recipient["username"];
            uint64_t recipientId = std::stoll(recipient["id"].get<string>());

            //Member & rec = privateChat.recipients[recipientId];
            privateChat.recipients.push_back(recipientId);
            Member & glob = AegisBot::createMember(recipientId);
            poco_trace_f1(*log, "User created: %Lu", recipientId);
            glob.id = recipientId;
            glob.avatar = recipientAvatar;
            glob.name = recipientName;
            glob.discriminator = recipientDiscriminator;
        }
    }
    sessionId = d["session_id"];
    json & userdata = d["user"];
    avatar = userdata["avatar"];
    discriminator = std::stoi(userdata["discriminator"].get<string>());
    userId = std::stoull(userdata["id"].get<string>());

    if (AegisBot::mention.size() == 0)
    {
        std::stringstream ss;
        ss << "<@" << userId  << ">";
        AegisBot::mention = ss.str();
    }

    username = userdata["username"];
    mfa_enabled = userdata["mfa_enabled"];
    active = true;
}

void AegisBot::onMessage(websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg)
{
    try
    {
        json result = json::parse(msg->get_payload());

        poco_trace_f1(*log, "Received JSON: %s", msg->get_payload());

        if (!result.is_null())
        {
            if (!result["t"].is_null())
            {
                string cmd = result["t"];
                poco_trace_f1(*log, "Processing: %s", cmd);
                if (cmd == "READY")
                {
                    processReady(result["d"]);
                }
                else if (cmd == "MESSAGE_CREATE")
                {
                    while (!active && isrunning)
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    userMessage(result["d"]);
                }
                else if (cmd == "GUILD_CREATE")
                {
                    loadGuild(result["d"]);

                    //load things like database commands and permissions here
                }
            }
            if (!result["s"].is_null())
                sequence = result["s"].get<uint64_t>();

            if (result["op"] == 9)
            {
                json obj = {
                    { "op", 2 },
                    {
                        "d",
                        {
                            { "token", cache->get("config:token") },
                            {
                                "properties",
                                {
                                    { "$os", "linux" },
                                    { "$browser", "aegisbot" },
                                    { "$device", "aegisbot" },
                                    { "$referrer", "" },
                                    { "$referring_domain", "" }
                                }
                            },
                            { "compress", false },
                            { "large_threshhold", 250 }
                        }
                    }
                };
                ws.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
            }
            if (result["op"] == 10)
            {
                uint64_t heartbeat = result["d"]["heartbeat_interval"];
                poco_trace_f1(*log, "Heartbeat timer added : %Lu ms", heartbeat);
                keepalive_timer_.expires_from_now(std::chrono::milliseconds(heartbeat));
                keepalive_timer_.async_wait([heartbeat, this](const boost::system::error_code & ec) { keepalive(ec, heartbeat); });
            }
            if (result["op"] == 11)
            {
                //heartbeat ACK
                poco_trace(*log, "Heartbeat ACK");
            }
        }
    }
    catch (Poco::BadCastException& e)
    {
        poco_error_f1(*log, "BadCastException: %s", (string)e.what());
    }
    catch (std::exception& e)
    {
        poco_error_f1(*log, "Failed to process object: %s", (string)e.what());
    }
    catch (...)
    {
        poco_error(*log, "Failed to process object: Unknown error");
    }
}

void AegisBot::userMessage(json & obj)
{
    json author = obj["author"];

    uint64_t userid = std::stoll(author["id"].get<string>());
    string username = author["username"];

    uint64_t channel_id = std::stoll(obj["channel_id"].get<string>());
    uint64_t id = std::stoll(obj["id"].get<string>());
    string content = obj["content"];

    //process chat

    //look for command start. this can be configurable per server
    //processing of the entire string won't occur for now unless
    //this matches
    //options can be added later to enable full scanning on a
    //per-channel basis for things like word filters etc

    Channel & channel = getChannel(channel_id);

    channel.guild().processMessage(obj);

    boost::tokenizer<boost::char_separator<char>> tok{ content, boost::char_separator<char>(" ") };

}

void AegisBot::keepalive(const boost::system::error_code& error, const uint64_t ms)
{
    if (error != boost::asio::error::operation_aborted)
    {
        json obj;
        obj["d"] = sequence;
        obj["op"] = 1;

#ifdef _TRACE
        std::cout << "Sending Heartbeat: " << obj.dump() << "\n";
#endif
        ws.send(connection, obj.dump(), websocketpp::frame::opcode::text);

        poco_trace_f1(*log, "Heartbeat timer added: %Lu ms", ms);
        keepalive_timer_.expires_from_now(std::chrono::milliseconds(ms));
        keepalive_timer_.async_wait([ms, this](const boost::system::error_code & ec) { keepalive(ec, ms); });
    }
}

void AegisBot::onConnect(websocketpp::connection_hdl hdl)
{
    this->hdl = hdl;
    std::cout << "Connection established.\n";

    json obj;

    if (sessionId.size() > 0)
    {
        //reconnect
        obj = {
            { "op", 6 },
            {
                "d",
                {
                    { "token", cache->get("config:token") },
                    { "session_id", sessionId },
                    { "seq", sequence }
                }
            }
        };
    }
    else
    {
        //new connect
        obj = {
            { "op", 2 },
            {
                "d",
                {
                    { "token", cache->get("config:token") },
                    {
                        "properties",
                        {
                            { "$os", "linux" },
                            { "$browser", "aegisbot" },
                            { "$device", "aegisbot" },
                            { "$referrer", "" },
                            { "$referring_domain", "" }
                        }
                    },
                    { "compress", false },
                    { "large_threshhold", 250 }
                }
            }
        };
    }

    ws.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
}

std::pair<bool,string> AegisBot::call(string url, string obj, RateLimits * endpoint /*= nullptr*/, string method /*= "GET"*/, string query /*= ""*/)
{
    uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    try
    {
        URI uri("https://discordapp.com/api/v6" + url);
        std::string path(uri.getPathAndQuery());

        HTTPSClientSession session(uri.getHost(), uri.getPort());
        HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
#ifdef SELFBOT
        request.set("Authorization", token);
#else
        request.set("Authorization", string("Bot ") + token);
#endif
        request.set("User-Agent", "DiscordBot (https://github.com/zeroxs/aegisbot 0.1)");
        request.set("Content-Type", "application/json");


        request.setMethod(method);

        if (obj.length() > 0)
        {
            request.setContentLength(obj.length());


#ifdef DEBUG_OUTPUT
            //std::cout << "Sent JSON: " << obj << "\n";

            std::ostringstream debugoutput;
            std::cout << "Sent request: ";
            request.write(std::cout);
            std::cout << std::endl;
#endif
            std::ostream & endp = session.sendRequest(request);

            endp << obj;
        }
        else
        {
            session.sendRequest(request);
        }

        HTTPResponse response;

        std::istream& rs = session.receiveResponse(response);
        HTTPResponse::HTTPStatus status = response.getStatus();

        std::cout << "status: " << (int)status << " " << response.getReason() << std::endl;

#ifdef DEBUG_OUTPUT
        std::ostringstream responseout;
        std::cout << "Response: ";
        response.write(std::cout);
        std::cout << std::endl;
#endif
        string result;

        Poco::StreamCopier::copyToString(rs, result);
#ifdef DEBUG_OUTPUT
        std::cout << "Result: " << result << "\n";
#endif

        if (endpoint)
        {
            endpoint->resetFailure();
            if (response.has("X-RateLimit-Global"))
            {
                rate_global = true;
                endpoint->rateRetry(Poco::DynamicAny(response.get("Retry-After")).convert<uint32_t>());
                log->error(Poco::format("Global rate limit reached. Pausing for %Lums", endpoint->rateRetry()));
                std::this_thread::sleep_for(std::chrono::milliseconds(endpoint->rateRetry()));
                rate_global = false;
                if (status == 429)
                {
                    return { false, "" };
                }
            }
            else
            {
                if (response.has("X-RateLimit-Limit"))
                    endpoint->rateLimit(Poco::DynamicAny(response.get("X-RateLimit-Limit")).convert<uint32_t>());
                if (response.has("X-RateLimit-Remaining"))
                    endpoint->rateRemaining(Poco::DynamicAny(response.get("X-RateLimit-Remaining")).convert<uint32_t>());
                if (response.has("X-RateLimit-Reset"))
                    endpoint->rateReset(Poco::DynamicAny(response.get("X-RateLimit-Reset")).convert<uint32_t>());

                if (status == 429)
                {
                    return { false, "" };
                }
#ifdef DEBUG_OUTPUT
                poco_trace_f1(*log, "rate_limit:     %u", endpoint->rateLimit());
                poco_trace_f1(*log, "rate_remaining: %u", endpoint->rateRemaining());
                poco_trace_f1(*log, "rate_reset:     %u", endpoint->rateReset());
                poco_trace_f1(*log, "epoch:          %Lu", epoch);
                poco_trace_f1(*log, "content:        %s", obj);
#endif
                log->information(Poco::format("Rates: %u:%u resets in: %Lus", endpoint->rateLimit(), endpoint->rateRemaining(), (endpoint->rateReset() > 0) ? (endpoint->rateReset() - epoch) : 0));
            }
        }

        return { true, result };
    }
    catch (std::exception&e)
    {
        log->error(Poco::format("Unhandled error in Bot::call(): %s", (string)e.what()));
        endpoint->addFailure();
    }

    return { false, "" };
}

void AegisBot::run()
{
    while (isrunning)
    {
        {
            std::lock_guard<std::recursive_mutex> lock(m);
            for (auto & guild : guildlist)
            {
                for (auto & channel : guild.second->channellist)
                {
                    if (channel.second->ratelimits.outqueue.size() > 0)
                    {
                        if (channel.second->ratelimits.rateRemaining())
                        {
                            if (channel.second->ratelimits.isFailureTime())
                            {
                                continue;
                            }

                            //TODO: support differing rate limits eg, differentiate between self commands and guild user renames
                            //TODO: also merge this + guild call() calls.

                            //Channel api calls
                            uint32_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                            auto message = channel.second->ratelimits.getMessage();
                            bool success = false;


                            {
                                //lock and pop when success
                                //std::lock_guard<std::recursive_mutex> lock(channel.second.ratelimits.m);

                                boost::tie(success, message.content) = call(message.endpoint, message.content, &message.channel().ratelimits, message.method, message.query);
                                try
                                {
                                    if (message.content.size() != 0)
                                        message.obj = json::parse(message.content);
                                }
                                catch (...)
                                {
                                    //dummy catch on empty or malformed responses
                                }

                                if (!success)
                                {
                                    //rate limit hit
                                    poco_trace_f4(*log, "Rate Limit hit or connection error - requeuing message [%s] [%Lu] [%s] [%Lu]", message.channel().guild().name, message.channel().guild().id, message.channel().name, message.channel().id);
                                    continue;
                                }
                                //message success, pop
                                message.channel().ratelimits.outqueue.pop();
                            }

                            poco_trace_f2(*log, "Message sent: [%s] [%s]", message.endpoint, message.content);
#ifdef DEBUG_OUTPUT
                            poco_trace_f1(*log, "rate_limit:     %u", message.channel().ratelimits.rateLimit());
                            poco_trace_f1(*log, "rate_remaining: %u", message.channel().ratelimits.rateRemaining());
                            poco_trace_f1(*log, "rate_reset:     %u", message.channel().ratelimits.rateReset());
                            poco_trace_f1(*log, "epoch:          %u", epoch);
                            poco_trace_f1(*log, "content:        %s", message.content);
#endif
                            if (message.callback)
                            {
                                message.callback(message);
                            }
                        }
                    }
                }

                if (guild.second->ratelimits.outqueue.size() > 0)
                {
                    if (guild.second->ratelimits.rateRemaining())
                    {
                        if (guild.second->ratelimits.isFailureTime())
                        {
                            continue;
                        }

                        //Guild api calls
                        uint32_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                        auto & message = guild.second->ratelimits.getMessage();
                        bool success = false;
                        {
                            //std::lock_guard<std::recursive_mutex> lock(guild.second->ratelimits.m);
                            boost::tie(success, message.content) = call(message.endpoint, message.content, &guild.second->ratelimits, message.method, message.query);
                            try
                            {
                                if (message.content.size() != 0)
                                    message.obj = json::parse(message.content);
                            }
                            catch (...)
                            {
                                //dummy catch on empty or malformed responses
                            }

                            if (!success)
                            {
                                //rate limit hit
                                poco_trace_f4(*log, "Rate Limit hit or connection error - requeuing message [%s] [%Lu] [%s] [%Lu]", message.channel().guild().name, message.channel().guild().id, message.channel().name, message.channel().id);
                                continue;
                            }
                            guild.second->ratelimits.outqueue.pop();
                        }
                        poco_trace_f2(*log, "Message sent: [%s] [%s]", message.endpoint, message.content);
#ifdef DEBUG_OUTPUT
                        poco_trace_f1(*log, "rate_limit:     %u", message.guild().ratelimits.rateLimit());
                        poco_trace_f1(*log, "rate_remaining: %u", message.guild().ratelimits.rateRemaining());
                        poco_trace_f1(*log, "rate_reset:     %u", message.guild().ratelimits.rateReset());
                        poco_trace_f1(*log, "epoch:          %u", epoch);
                        poco_trace_f1(*log, "content:        %s", message.content);
#endif
                        if (message.callback)
                        {
                            message.callback(message);
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

/*
template <typename T, typename... _BoundArgs>
void AegisBot::createTimer(uint64_t t, shared_ptr<boost::asio::steady_timer> timer, T f, _BoundArgs&&... __args)
{
    //new timer
    if (timer == nullptr)
        timer = make_shared<shared_ptr<boost::asio::steady_timer>::element_type>(io_service);

    timer->expires_from_now(std::chrono::milliseconds(t));
    timer->async_wait(std::bind(f, this, __args...));
    poco_trace_f1(*log, "createTimer(%Lu)", t);
}*/

void AegisBot::wssend(string obj)
{
    std::lock_guard<std::recursive_mutex> lock(wsq);
    //TODO check rate limits here and pop if send
    {
        ws.send(hdl, obj, websocketpp::frame::opcode::text);
    }
}

void AegisBot::loadGuild(json & obj)
{
    std::lock_guard<std::recursive_mutex> lock(m);

    //uint64_t application_id = obj->get("application_id").convert<uint64_t>();
    uint64_t id = std::stoll(obj["id"].get<string>());


    Guild & guild = createGuild(id);
    try
    {
        poco_trace_f1(*log, "Guild created: %Lu", id);
        guild.id = id;

        //guild->cmdlist = defaultcmdlist;

#define GET_NULL(x,y) (x[y].is_null())?"":x[y]
        guild.name = GET_NULL(obj, "name");
        guild.icon = GET_NULL(obj, "icon");
        guild.splash = GET_NULL(obj, "splash");
        guild.owner_id = std::stoll(obj["owner_id"].get<string>());
        guild.region = obj["region"];
        guild.afk_channel_id = obj["afk_channel_id"].is_null() ? 0 : std::stoll(obj["afk_channel_id"].get<string>());
        guild.afk_timeout = obj["afk_timeout"];//in seconds
        guild.embed_enabled = obj.count("embed_enabled") ? obj["embed_enabled"].get<bool>() : false;
        //guild.embed_channel_id = obj->get("embed_channel_id").convert<uint64_t>();
        guild.verification_level = obj["verification_level"];
        guild.default_message_notifications = obj["default_message_notifications"];
        guild.mfa_level = obj["mfa_level"];
        guild.joined_at = obj["joined_at"];
        guild.large = obj["large"];
        guild.unavailable = obj.count("unavailable")?obj["unavailable"].get<bool>():true;
        guild.member_count = obj["member_count"];
        json voice_states = obj["voice_states"];
        json members = obj["members"];
        json channels = obj["channels"];
        json presences = obj["presences"];
        json roles = obj["roles"];
        json emojis = obj["emojis"];
        json features = obj["features"];

        for (auto & channel : channels)
        {
            loadChannel(channel, id);
        }

        for (auto & member : members)
        {
            loadMember(member, guild);
        }

        for (auto & role : roles)
        {
            loadRole(role, guild);
        }

        for (auto & presence : presences)
        {
            loadPresence(presence, guild);
        }

        for (auto & emoji : emojis)
        {
            loadEmoji(emoji, guild);
        }

        for (auto & feature : features)
        {
            //??
        }

        for (auto & voicestate : voice_states)
        {
            //no voice yet
        }



    }
    catch(std::exception&e)
    {
        poco_error_f2(*log, "Error processing guild[%Lu] %s", id, (string)e.what());
    }
}

void AegisBot::loadChannel(json & channel, uint64_t guild_id)
{
    uint64_t channel_id = std::stoll(channel["id"].get<string>());
    Guild & guild = getGuild(guild_id);

    try
    {
        //does channel exist?
        //TODO: these 'leak' memory in the sense that they create a shared_ptr entry set to empty
        //doesn't really hurt, and shouldn't really add up to anything to be concerned about, but maybe
        //do a check for .count() on all the checks instead?
        Channel & checkchannel = createChannel(channel_id, guild_id);
        poco_trace_f2(*log, "Channel[%Lu] created for guild[%Lu]", channel_id, guild_id);
        checkchannel.name = GET_NULL(channel, "name");
        checkchannel.position = channel["position"];
        checkchannel.type = (ChannelType)channel["type"].get<int>();// 0 = text, 2 = voice

                                                                     //voice channels

        if (channel.find("bitrate") != channel.end())
        {
            checkchannel.bitrate = channel["bitrate"];
            checkchannel.user_limit = channel["user_limit"];
        }
        else
        {
            //not a voice channel, so has a topic field and last message id
            checkchannel.topic = GET_NULL(channel, "topic");
            checkchannel.last_message_id = (channel["last_message_id"].is_null()) ? 0 : std::stoll(channel["last_message_id"].get<string>());
        }


        json permission_overwrites = channel["permission_overwrites"];
        for (auto & permission : permission_overwrites)
        {
            uint32_t allow = permission["allow"];
            uint32_t deny = permission["deny"];
            uint64_t p_id = std::stoll(permission["id"].get<string>());
            string p_role = GET_NULL(permission, "role");
            //TODO: implement
        }

        guild.channellist.insert(std::pair<uint64_t, Channel*>(checkchannel.id, &checkchannel));
    }
    catch (std::exception&e)
    {
        poco_error_f3(*log, "Error processing channel[%Lu] of guild[%Lu] %s", channel_id, guild_id, (string)e.what());
    }
}

void AegisBot::loadMember(json & member, Guild & guild)
{
    uint64_t guildId = guild.id;

    json user = member["user"];
    uint64_t member_id = std::stoll(user["id"].get<string>());
    try
    {
        Member & checkmember = createMember(member_id);
        poco_trace_f2(*log, "Member[%Lu] created for guild[%Lu]", member_id, guildId);
        guild.clientlist[member_id] = std::pair<Member*, uint16_t>(&checkmember, 0);

        checkmember.avatar = GET_NULL(user, "avatar");
        checkmember.discriminator = std::stoll(user["discriminator"].get<string>());
        checkmember.name = GET_NULL(user, "username");

        checkmember.deaf = member["deaf"];
        checkmember.joined_at = member["joined_at"];
        checkmember.mute = member["mute"];
        checkmember.isbot = member["bot"].is_null() ? false : true;
        checkmember.guilds[guildId].nickname = GET_NULL(member, "nick");

        json roles = member["roles"];
        for (auto & r : roles)
            checkmember.guilds[guildId].roles.push_back(std::stoll(r.get<string>()));

        checkmember.guilds[guildId].guild = &getGuild(guildId);
    }
    catch (std::exception&e)
    {
        poco_error_f3(*log, "Error processing member[%Lu] of guild[%Lu] %s", member_id, guildId, (string)e.what());
    }
}

void AegisBot::loadRole(json & role, Guild & guild)
{
    uint64_t guildId = guild.id;

    Role _role;
    uint64_t role_id = std::stoll(role["id"].get<string>());
    try
    {
        _role.hoist = role["hoist"];
        _role.managed = role["managed"];
        _role.mentionable = role["mentionable"];
        _role.permissions = role["permissions"];
        _role.position = role["position"];
        _role.name = GET_NULL(role, "name");
        _role.color = role["color"];
        guild.rolelist.insert(std::pair<uint64_t, Role>(role_id, std::move(_role)));


//         for (auto & member : guild->clientlist)
//         {
//             for (auto & role : member.second.first->roles)
//             {
//                 if (role == role_id)
//                 {
//                     //matches
//                 }
//             }
//         }
    }
    catch (std::exception&e)
    {
        poco_error_f3(*log, "Error processing role[%Lu] of guild[%Lu] %s", role_id, guildId, (string)e.what());
    }
}

void AegisBot::loadEmoji(json & member, Guild & guild)
{
    return;
}

void AegisBot::loadPresence(json & member, Guild & guild)
{
    return;
}
