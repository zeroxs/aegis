//
// Bot.cpp
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

#ifdef USE_REDIS
#include "ABRedisCache.h"
#endif

#include <boost/tokenizer.hpp>

AegisBot * AegisBot::_instance = nullptr;

AegisBot::AegisBot()
    : keepalive_timer_(io_service)
{
    pFC = new FormattingChannel(new PatternFormatter("%p:%T %t"));
    pFC->setChannel(new ConsoleChannel);
    pFC->open();

    File f("log/");
    if (!f.exists())
    {
        f.createDirectory();
    }
    else if (f.isFile())
    {
        throw std::runtime_error("Error creating log directory!");
    }

    pFCf = new FormattingChannel(new PatternFormatter("%Y-%m-%d %H:%M:%S.%c | %s:%q:%t"));
    pFCf->setChannel(new FileChannel("log/console.log"));
    pFCf->setProperty("rotation", "daily");
    pFCf->setProperty("times", "local");
    pFCf->open();
    logf = &Poco::Logger::create("fileLogger", pFCf, Message::PRIO_TRACE);
    log = &Poco::Logger::create("consoleLogger", pFC, Message::PRIO_TRACE);
}

AegisBot::~AegisBot()
{
    Poco::Logger::shutdown();
    pFCf->close();
    pFC->close();
}

boost::shared_ptr<Guild> AegisBot::CreateGuild(uint64_t id)
{
    std::lock_guard<std::mutex> lock(AegisBot::GetSingleton().m);
    if (_instance == nullptr)
        throw std::runtime_error("Cannot create a guild when no bot instance exists.");
    if (AegisBot::GetSingleton().guildlist.count(id))
        return AegisBot::GetSingleton().guildlist[id];
    boost::shared_ptr<Guild> guild = boost::make_shared<Guild>();
    AegisBot::GetSingleton().guildlist[id] = guild;
    guild->id = id;
    return guild;
}

void AegisBot::setup_cache(ABCache * in)
{
    cache = in;
}

void AegisBot::loadConfigs()
{
    //TODO: might need to add a mutex here to prevent actions from running while configs reload
#ifdef USE_REDIS
    int32_t level = boost::lexical_cast<int32_t>(cache->get("config:loglevel"));
    token = cache->get("config:token");
    logf->setLevel(level);
    log->setLevel(level);
#endif
}

bool AegisBot::initialize(uint64_t shardid, uint64_t maxshard)
{
    this->shardid = shardid;
    this->shardidmax = maxshard;
    //obtain data from cache (redis)
    
    starttime = std::chrono::steady_clock::now();


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
    string res;
    call("/gateway", &res, nullptr, "GET", "");
    json ret = json::parse(res);
    gatewayurl = ret["url"];
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

void AegisBot::connectWS()
{
    //make a portable way to do this even though I like the atomic-ness of redis scripts for obtaining inter-process locks
#ifdef USE_REDIS
    while (true)
    {
        uint64_t epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        string obtainlock = Poco::format("\"local time = redis.call('get', KEYS[1]) if (time == nil) then redis.call('set', KEYS[1], ARGV[1]) return 1 end if (time < ARGV[1]) then return 0 else redis.call('set', KEYS[1], ARGV[1]) return 1 end\" 1 config:wslock %Lu", epoch);
        if (static_cast<ABRedisCache*>(cache)->eval(obtainlock) == "1")
            break;
        else
        {
            log->warning("Websocket lock held. Waiting to reconnect.");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

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
    connectWS();
}

void AegisBot::processReady(json & d)
{
    std::lock_guard<std::mutex> lock(m);
    json guilds = d["guilds"];
    for (auto & guildobj : guilds)
    {
        uint64_t id = std::stoll(guildobj["id"].get<string>());
        bool unavailable = guildobj["unavailable"];

        auto guild = guildlist[id];
        
        if (guild == nullptr)
        {
            guildlist[id] = boost::make_shared<Guild>();
            poco_trace_f1(*log, "Guild created: %Lu", id);
        }
        guildlist[id]->unavailable = unavailable;
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
        uint64_t last_message_id = std::stoll(channel["last_message_id"].get<string>());
        int32_t channelType = channel["type"];
        json recipients = channel["recipients"];
      
        auto privateChat = private_channels[channel_id];
        if (privateChat == nullptr)
        {
            privateChat = boost::make_shared<PrivateChat>();
            private_channels[channel_id] = privateChat;
            poco_trace_f1(*log, "Private Channel created: %Lu", channel_id);
        }
        
        for (auto & recipient : recipients)
        {
            string recipientAvatar = recipient["avatar"];
            uint16_t recipientDiscriminator = std::stoll(recipient["discriminator"].get<string>());
            string recipientName = recipient["username"];
            uint64_t recipientId = std::stoll(recipient["id"].get<string>());

            auto rec = privateChat->recipients[recipientId];
            auto glob = globalusers[recipientId];
            if (glob == nullptr)
            {
                //global user doesn't exist
                if (rec == nullptr)
                {
                    //new entry entirely
                    globalusers[recipientId] = privateChat->recipients[recipientId] = rec = boost::make_shared<Member>();
                    poco_trace_f1(*log, "User created: %Lu", recipientId);
                }
                else
                {
                    //BUG: channel should not contain a user that does not exist in the global cache
                    poco_error_f2(*log, "Channel[%Lu] user[%Lu] has no global cache entry", channel_id, recipientId);
                    globalusers[recipientId] = rec;
                }
            }
            if (rec == nullptr)
            {
                //global cache has an entry, channel does not
                privateChat->recipients[recipientId] = rec = glob;
            }
            rec->id = recipientId;
            rec->avatar = recipientAvatar;
            rec->name = recipientName;
            rec->discriminator = recipientDiscriminator;
        }
    }
    sessionId = d["session_id"];
    json & userdata = d["user"];
    avatar = userdata["avatar"];
    discriminator = std::stoll(userdata["discriminator"].get<string>());
    userId = std::stoll(userdata["id"].get<string>());
    username = userdata["username"];
    mfa_enabled = userdata["mfa_enabled"];
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

    auto channel = channellist[channel_id];
    if (channel == nullptr)
    {
        poco_error_f2(*log, "Chat message [%Lu] on has no channel entry [%Lu]", id, channel_id);
        return;
    }
    auto guild = channel->belongs_to();

    if (guild == nullptr)
    {
        poco_error_f2(*log, "Chat message [%Lu] on channel [%Lu] has no guild entry", id, channel_id);
        return;
    }

    //TODO: add some core bot management
    if ((content == "?exit") && (userid == 171000788183678976LL))
    {
        //
        poco_critical_f3(*log, "Bot shutdown g[%Lu] c[%Lu] u[%Lu]", id, channel_id, userid);
        ws.stop();
        io_service.stop();
        return;
    }

    guild->processMessage(obj);

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
    std::cout << "Connection established.\n";

    json obj = {
        { "op", 2 },
        {
            "d",
            {
                { "token", token },
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
                {"compress", false},
                {"large_threshhold", 250}
            }
        }
    };
#ifdef _TRACE
    std::cout << "Client Handshake:\n" << obj.dump() << "\n";
#endif
    ws.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
}

bool AegisBot::call(string url, string * obj /*= nullptr*/, RateLimits * endpoint /*= nullptr*/, string method /*= "GET"*/, string query /*= ""*/)
{
    uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    try
    {
        URI uri("https://discordapp.com/api/v6" + url);
        std::string path(uri.getPathAndQuery());

        HTTPSClientSession session(uri.getHost(), uri.getPort());
        HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        request.set("Authorization", string("Bot ") + token);
        request.set("User-Agent", "DiscordBot (https://github.com/zeroxs/aegisbot 0.1)");
        request.set("Content-Type", "application/json");


        if (obj && obj->length() > 0)
        {
            request.setMethod("POST");
            request.setContentLength(obj->length());

            std::cout << "Sent JSON: " << *obj << "\n";

#ifdef DEBUG_OUTPUT
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

        if (obj)
        {
            *obj = "";
            Poco::StreamCopier::copyToString(rs, *obj);
#ifdef DEBUG_OUTPUT
            std::cout << "Result: " << *obj << "\n";
#endif
        }

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
                    return false;
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
                    return false;
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

        return true;
    }
    catch (std::exception&e)
    {
        log->error(Poco::format("Unhandled error in Bot::call(): %s", (string)e.what()));
        endpoint->addFailure();
    }

    return false;
}

void AegisBot::run()
{
    //some more init stuff
    boost::asio::io_service::work work(io_service);

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

    for (size_t t = 0; t < std::thread::hardware_concurrency() * 2; t++)
    {
        threadPool.push_back(std::thread([&]() { io_service.run(); }));
    }


    while (isrunning)
    {
        {
            std::lock_guard<std::mutex> lock(m);
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

                            uint32_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                            auto message = channel.second->ratelimits.getMessage();
                            if (!call(message->endpoint, &message->content, &message->channel->ratelimits, message->method, message->query))
                            {
                                //rate limit hit, requeue message
                                message->channel->ratelimits.putMessage(message);
#ifdef DEBUG_OUTPUT
                                poco_trace_f4(*log, "Rate Limit hit or connection error - requeuing message [%s] [%Lu] [%s] [%Lu]", message->guild->name, message->guild->id, message->channel->name, message->channel->id);
#endif
                                continue;
                            }
                            poco_trace_f2(*log, "Message sent: [%s] [%s]", message->endpoint, message->content);
#ifdef DEBUG_OUTPUT
                            poco_trace_f1(*log, "rate_limit:     %u", message->channel->ratelimits.rateLimit());
                            poco_trace_f1(*log, "rate_remaining: %u", message->channel->ratelimits.rateRemaining());
                            poco_trace_f1(*log, "rate_reset:     %u", message->channel->ratelimits.rateReset());
                            poco_trace_f1(*log, "epoch:          %u", epoch);
                            poco_trace_f1(*log, "content:        %s", message->content);
#endif
                            if (message->callback)
                            {
                                message->callback(message);
                            }
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

template <typename T, typename... _BoundArgs>
void AegisBot::createTimer(uint64_t t, shared_ptr<boost::asio::steady_timer> timer, T f, _BoundArgs&&... __args)
{
    //new timer
    if (timer == nullptr)
        timer = boost::make_shared<shared_ptr<boost::asio::steady_timer>::element_type>(io_service);

    timer->expires_from_now(std::chrono::milliseconds(t));
    timer->async_wait(std::bind(f, this, __args...));
    poco_trace_f1(*log, "createTimer(%Lu)", t);
}

shared_ptr<Guild> AegisBot::loadGuild(json & obj)
{
    std::lock_guard<std::mutex> lock(m);

    shared_ptr<Guild> guild;

    //uint64_t application_id = obj->get("application_id").convert<uint64_t>();
    uint64_t id = std::stoll(obj["id"].get<string>());

    try
    {
        guild = guildlist[id];
        if (guild == nullptr)
        {
            //not cached - create
            guildlist[id] = guild = boost::make_shared<Guild>();
            guild->id = id;
            poco_trace_f1(*log, "Guild created: %Lu", id);
        }
        else
            guild->id = id;

        guild->cmdlist = defaultcmdlist;

#define GET_NULL(x,y) (x[y].is_null())?"":x[y]
        guild->name = GET_NULL(obj, "name");
        guild->icon = GET_NULL(obj, "icon");
        guild->splash = GET_NULL(obj, "splash");
        guild->owner_id = std::stoll(obj["owner_id"].get<string>());
        guild->region = obj["region"];
        guild->afk_channel_id = obj["afk_channel_id"].is_null() ? 0 : std::stoll(obj["afk_channel_id"].get<string>());
        guild->afk_timeout = obj["afk_timeout"];//in seconds
        //guild->embed_enabled = obj->get("embed_enabled").convert<bool>();
        //guild->embed_channel_id = obj->get("embed_channel_id").convert<uint64_t>();
        guild->verification_level = obj["verification_level"];
        guild->default_message_notifications = obj["default_message_notifications"];
        guild->mfa_level = obj["mfa_level"];
        guild->joined_at = obj["joined_at"];
        guild->large = obj["large"];
        guild->unavailable = obj["unavailable"];
        guild->member_count = obj["member_count"];
        json voice_states = obj["voice_states"];
        json members = obj["members"];
        json channels = obj["channels"];
        json presences = obj["presences"];
        json roles = obj["roles"];
        json emojis = obj["emojis"];
        json features = obj["features"];

        for (auto & channel : channels)
        {
            uint64_t channel_id = std::stoll(channel["id"].get<string>());

            try
            {
                //does channel exist?
                //TODO: these 'leak' memory in the sense that they create a shared_ptr entry set to empty
                //doesn't really hurt, and shouldn't really add up to anything to be concerned about, but maybe
                //do a check for .count() on all the checks instead?
                auto checkchannel = channellist[channel_id];
                if (checkchannel == nullptr)
                {
                    //not cached - create
                    channellist[channel_id] = checkchannel = boost::make_shared<Channel>(guild);
                    checkchannel->id = channel_id;
                    checkchannel->belongs_to(guild);
                    poco_trace_f2(*log, "Channel[%Lu] created for guild[%Lu]", channel_id, id);
                }
                checkchannel->name = GET_NULL(channel, "name");
                checkchannel->position = channel["position"];
                checkchannel->type = (ChannelType)channel["type"].get<int>();// 0 = text, 2 = voice

                //voice channels
                
                if (channel.find("bitrate") != channel.end())
                {
                    checkchannel->bitrate = channel["bitrate"];
                    checkchannel->user_limit = channel["user_limit"];
                }
                else
                {
                    //not a voice channel, so has a topic field and last message id
                    checkchannel->topic = GET_NULL(channel, "topic");
                    checkchannel->last_message_id = (channel["last_message_id"].is_null()) ? 0 : std::stoll(channel["last_message_id"].get<string>());
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

                guild->channellist.insert(std::pair<uint64_t, shared_ptr<Channel>>(checkchannel->id, checkchannel));
                channellist.insert(std::pair<uint64_t, shared_ptr<Channel>>(checkchannel->id, checkchannel));
            }
            catch (std::exception&e)
            {
                poco_error_f3(*log, "Error processing channel[%Lu] of guild[%Lu] %s", channel_id, id, (string)e.what());
            }
        }

        for (auto & member : members)
        {
            json user = member["user"];
            uint64_t member_id = std::stoll(user["id"].get<string>());
            try
            {
                //does member exist?
                auto checkmember = globalusers[member_id];
                if (checkmember == nullptr)
                {
                    //not cached - create
                    globalusers[member_id] = checkmember = boost::make_shared<Member>();
                    checkmember->id = member_id;
                    poco_trace_f2(*log, "Member[%Lu] created for guild[%Lu]", member_id, id);
                }
                guild->clientlist[member_id] = std::pair<shared_ptr<Member>, uint16_t>(checkmember, 0);

                checkmember->avatar = GET_NULL(user, "avatar");
                checkmember->discriminator = std::stoll(user["discriminator"].get<string>());
                checkmember->name = GET_NULL(user, "username");

                checkmember->deaf = member["deaf"];
                checkmember->joined_at = member["joined_at"];
                checkmember->mute = member["mute"];
                checkmember->isbot = member["bot"].is_null()?false:true;
                checkmember->nick = GET_NULL(member, "nick");

                json roles = member["roles"];
                for (auto & r : roles)
                    checkmember->roles.push_back(std::stoll(r.get<string>()));




            }
            catch (std::exception&e)
            {
                poco_error_f3(*log, "Error processing member[%Lu] of guild[%Lu] %s", member_id, id, (string)e.what());
            }
        }

        for (auto  &role : roles)
        {
            shared_ptr<Role> _role = boost::make_shared<Role>();
            uint64_t role_id = std::stoll(role["id"].get<string>());
            try
            {
                _role->hoist = role["hoist"];
                _role->managed = role["managed"];
                _role->mentionable = role["mentionable"];
                _role->permissions = role["permissions"];
                _role->position = role["position"];
                _role->name = GET_NULL(role, "name");
                _role->color = role["color"];
                guild->rolelist.insert(std::pair<uint64_t, shared_ptr<Role>>(role_id, std::move(_role)));
            }
            catch (std::exception&e)
            {
                poco_error_f3(*log, "Error processing role[%Lu] of guild[%Lu] %s", role_id, id, (string)e.what());
            }
        }

        for (auto & presence : presences)
        {
        }

        for (auto & emoji : emojis)
        {
        }

        for (auto & feature : features)
        {
        }

        for (auto & voicestate : voice_states)
        {
        }
    }
    catch(std::exception&e)
    {
        poco_error_f2(*log, "Error processing guild[%Lu] %s", id, e.what());
    }

    return guild;
}

void AegisBot::info_command(shared_ptr<ABMessage> message)
{
    uint64_t timenow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    uint64_t guild_count = guildlist.size();
    uint64_t member_count = globalusers.size();
    uint64_t channel_count = channellist.size();
    uint64_t channel_text_count = 0;
    uint64_t channel_voice_count = 0;
    {
        std::lock_guard<std::mutex> lock(m);
        for (auto & channel : channellist)
        {
            if (channel.second->type == ChannelType::TEXT)
                channel_text_count++;
            else
                channel_voice_count++;
        }
    }
    std::stringstream members;
    members << member_count << "\n0 Online\n0 Offline\nstuff";

    std::stringstream channels;
    channels << channel_count << " total\n" << channel_text_count << " text\n" << channel_voice_count << " voice";

    std::stringstream guilds;
    guilds << guild_count;

    message->content = "";
    string uptime = uptime();
    string stats;
    stats = Poco::format("Memory usage: %.2fMB\nMax Memory: %.2fMB", double(getCurrentRSS()) / (1024 * 1024), double(getPeakRSS()) / (1024 * 1024));
    json t = {
        { "title", "AegisBot" },
        { "description", "[Latest bot source](https://github.com/zeroxs/aegisbot)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)" },
        { "color", 10599460 },
        { "fields",
        json::array(
    {
        { { "name", "Members" },{ "value", members.str() },{ "inline", true } },
        { { "name", "Channels" },{ "value", channels.str() },{ "inline", true } },
        { { "name", "Uptime test" },{ "value", uptime },{ "inline", true } },
        { { "name", "Guilds" },{ "value", guilds.str() },{ "inline", true } }
    }
            )
        },
        { "footer",{ { "icon_url", "https://cdn.discordapp.com/attachments/288707540844412928/289572000391888906/cpp.png" },{ "text", "Made in c++ running aegisbot library" } } }
    };
    t["description"] = stats;
    message->channel->sendMessageEmbed(json(), t);
}
