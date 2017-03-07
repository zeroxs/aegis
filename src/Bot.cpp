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

#include "Bot.h"
#include "Guild.h"

#include <boost/tokenizer.hpp>

Bot::Bot()
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

Bot::~Bot()
{
}

void Bot::setup_cache(ABCache * in)
{
    cache = in;
}

bool Bot::initialize()
{
    //obtain data from cache (redis)


    token = cache->get("ab:config:token");
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

    ws.set_message_handler(std::bind(&Bot::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    ws.set_open_handler(std::bind(&Bot::onConnect, this, std::placeholders::_1));
    ws.set_close_handler(std::bind(&Bot::onClose, this, std::placeholders::_1));

    websocketpp::lib::error_code ec;
    json ret = json::parse(call("/gateway"));
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

void Bot::onClose(websocketpp::connection_hdl hdl)
{
    keepalive_timer_.cancel();
    poco_error(*log, "Connection closed.");
    //TODO: throttle reconnects to prevent being denied due to spam
    poco_information(*log, "Reconnecting.");
    websocketpp::lib::error_code ec;
    connection = ws.get_connection(gatewayurl + "/?encoding=json&v=6", ec);
    ws.connect(connection);
}

void Bot::processReady(json & d)
{
    json guilds = d["guilds"];
    for (auto & guildobj : guilds)
    {
        uint64_t id = guildobj["id"];
        bool unavailable = guildobj["unavailable"];

        auto guild = guildlist[id];
        
        if (guild == nullptr)
        {
            guildlist[id] = boost::make_shared<Guild>(*this);
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
        uint64_t channel_id = channel["id"];
        uint64_t last_message_id = channel["last_message_id"];
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
            uint16_t recipientDiscriminator = recipient["discriminator"];
            string recipientName = recipient["username"];
            uint64_t recipientId = recipient["id"];

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
    discriminator = userdata["discriminator"];
    userId = userdata["id"];
    username = userdata["username"];
    mfa_enabled = userdata["mfa_enabled"];
}
//    "d":{  "presences":[], "relationships" : [], "user_settings" : {}, "v" : 6}, "op" : 0, "s" : 1, "t" : "READY"
void Bot::onMessage(websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg)
{
    try
    {
        json result = json::parse(msg->get_payload());

        poco_trace_f1(*log, "JSON: %s", msg->get_payload());

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
                }
            }
            if (!result["s"].is_null())
                sequence = result["s"].get<uint64_t>();

            if (result["op"] == 10)
            {
                uint64_t heartbeat = result["d"]["heartbeat_interval"];
                poco_trace_f1(*log, "Heartbeat timer added : %Lu ms", heartbeat);
                keepalive_timer_.expires_from_now(std::chrono::milliseconds(heartbeat));
                keepalive_timer_.async_wait([&](const boost::system::error_code & ec) { keepalive(ec, heartbeat); });
            }
            if (result["op"] == 11)
            {
                //heartbeat ACK
                poco_trace(*log, "Heartbeat ACK");
            }
        }
//     JSON: 
//     { 
//         "d":
//         {
//             "attachments":[],
//             "edited_timestamp" : null,
//             "embeds" : [],
//             "mention_everyone" : false,
//             "mention_roles" : [],
//             "mentions" : [],
//             "timestamp" : "2017-02-28T15:22:25.955000+00:00",
//             "type" : 0
//             //"webhook_id": ""
//         },
//         "op" : 0,
//         "s" : 4,
//         "t" : "MESSAGE_CREATE"
//     }
//         std::ostringstream out;
//         result->stringify(out);
//         std::cout << "JSON: " << out.str() << "\n";
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

void Bot::userMessage(json & obj)
{
    json author = obj["author"];

    uint64_t userid = author["id"];
    string username = author["username"];

    uint64_t channel_id = obj["channel_id"];
    uint64_t id = obj["id"];
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
    if ((content == "?exit") && (userid == 171000788183678976L))
    {
        //
        poco_critical_f3(*log, "Bot shutdown g[%Lu] c[%Lu] u[%Lu]", id, channel_id, userid);
        ws.stop();
        io_service.stop();
        return;
    }

    size_t len = guild->prefix.length();
        
    if (content.substr(0, len) == guild->prefix)
    {
        //match, process whole message
        guild->processMessage(obj);
    }

    boost::tokenizer<boost::char_separator<char>> tok{ content, boost::char_separator<char>(" ") };

}

void Bot::keepalive(const boost::system::error_code& error, const uint64_t ms)
{
    if (error != boost::asio::error::operation_aborted)
    {
        json obj;
        obj["d"] = sequence;
        obj["op"] = 1;

#ifdef _TRACE
        std::cout << "Sending Heartbeat: " << out.str() << "\n";
#endif
        ws.send(connection, obj.dump(-1), websocketpp::frame::opcode::text);

        poco_trace_f1(*log, "Heartbeat timer added: %Lu ms", ms);
        keepalive_timer_.expires_from_now(std::chrono::milliseconds(ms));
        keepalive_timer_.async_wait([&](const boost::system::error_code & ec) { keepalive(ec, ms); });
    }
}

void Bot::onConnect(websocketpp::connection_hdl hdl)
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
                }
            }
        }
    };
//     obj["op"] = 2;
//     json d;
//     d["token"] = token;
//     json properties;
//     properties["$os"] = "linux";
//     properties["$browser"] = "discordcpp";
//     properties["$device"] = "discordcpp";
//     properties["$referrer"] = "";
//     properties["$referring_domain"] = "";
//     d["properties"] = properties;
//     d["compress"] = false;
//     d["large_threshold"] = 250;
//     obj["d"] = d;
#ifdef _TRACE
    //std::cout << "Client Handshake:\n" << out.str() << "\n";
#endif
    ws.send(hdl, obj.dump(-1), websocketpp::frame::opcode::text);
}

string Bot::call(string url, string obj, EndpointHint endpointHint, string method, string query, shared_ptr<boost::asio::steady_timer> timer)
{
    uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    try
    {
        URI uri("https://discordapp.com/api/v6" + url);
        std::string path(uri.getPathAndQuery());

        HTTPSClientSession session(uri.getHost(), uri.getPort());
        HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        request.set("Authorization", string("Bot ") + token);
        request.set("User-Agent", "DiscordBot (https://github.com/zeroxs/discordcpp, 0.1)");
        request.set("Content-Type", "application/json");


        if (obj.length() > 0)
        {
            request.setMethod("POST");
            request.setContentLength(obj.length());

            std::cout << "JSON: " << obj << "\n";

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

        std::string output;
        Poco::StreamCopier::copyToString(rs, output);
        std::cout << "Result: " << output << "\n";

        if (response.has("X-RateLimit-Global"))
        {
            rate_global = true;
            rateLimits[endpointHint].retry_after = Poco::DynamicAny(response.get("Retry-After")).convert<uint32_t>();
            log->error(Poco::format("Global rate limit reached. Pausing for %Lums", rateLimits[endpointHint].retry_after));
            std::this_thread::sleep_for(std::chrono::milliseconds(rateLimits[endpointHint].retry_after));
            rate_global = false;
        }
        else
        {
            if (response.has("X-RateLimit-Limit"))
                rateLimits[endpointHint].rate_limit = Poco::DynamicAny(response.get("X-RateLimit-Limit")).convert<uint32_t>();
            if (response.has("X-RateLimit-Remaining"))
                rateLimits[endpointHint].rate_remaining = Poco::DynamicAny(response.get("X-RateLimit-Remaining")).convert<uint32_t>();
            if (response.has("X-RateLimit-Reset"))
                rateLimits[endpointHint].rate_reset = Poco::DynamicAny(response.get("X-RateLimit-Reset")).convert<uint32_t>();

#ifdef DEBUG_OUTPUT
            poco_trace_f1(*log, "rate_limit:     %u", rateLimits[endpointHint].rate_limit);
            poco_trace_f1(*log, "rate_remaining: %u", rateLimits[endpointHint].rate_remaining);
            poco_trace_f1(*log, "rate_reset:     %Lu", rateLimits[endpointHint].rate_reset);
            poco_trace_f1(*log, "epoch:          %Lu", epoch);
#endif
            log->information(Poco::format("Rates: %u:%u resets in: %Lums", rateLimits[endpointHint].rate_limit, rateLimits[endpointHint].rate_remaining, (rateLimits[endpointHint].rate_reset > 0) ? (rateLimits[endpointHint].rate_reset - epoch) : 0));
        }

        return output;
    }
    catch (std::exception&e)
    {
        log->error(Poco::format("Unhandled error in Bot::call(): %s", e.what()));
    }

    return "";
}

void Bot::sendMessage(string content, uint64_t channel, shared_ptr<boost::asio::steady_timer> timer)
{
    uint64_t epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (!rateLimitCheck(epoch, EndpointHint::CHANNEL))
    {
        //rate limit reached, create timer
        createTimer(rateLimits[EndpointHint::CHANNEL].rate_reset - epoch, timer, std::mem_fn(&Bot::sendMessage), content, channel);
        return;
    }

    poco_trace(*log, "sendMessage() goes through");
    json obj;
    obj["content"] = content;
    call(Poco::format("/channels/%Lu/messages", channel), obj.dump(-1), EndpointHint::CHANNEL);
}

void Bot::bulkDelete(uint64_t channel, std::vector<string> messages)
{
    json arr(messages);
    json obj;
    obj["messages"] = arr;
    call(Poco::format("/channels/%Lu/messages/bulk-delete", channel), obj.dump(-1), EndpointHint::CHANNEL, "POST");
    tempmessages.clear();
}
void Bot::getMessages(uint64_t channel, uint64_t messageid)
{
    string res = call(Poco::format("/channels/%Lu/messages", channel), "", EndpointHint::CHANNEL, "GET", Poco::format("?before=%Lu&limit=100", messageid));

    json arr = json::parse(res);

    for (auto & m : arr)
    {
        string entry = m["id"];
        poco_trace_f1(*log, "Message entry: %s", entry);
        tempmessages.push_back(entry);
    }

}

bool Bot::rateLimitCheck(uint64_t epoch, EndpointHint endpointHint)
{
    if (rateLimits[endpointHint].rate_remaining == 0)
    {
        if (rateLimits[endpointHint].rate_reset < epoch)
        {
            //reset rates
            rateLimits[endpointHint].rate_reset = 0;
            rateLimits[endpointHint].rate_remaining = rateLimits[endpointHint].rate_limit;
            return true;
        }
        //not enough rate left, queue this message
        return false;
    }
    return true;
}

template <typename T, typename... _BoundArgs>
void Bot::createTimer(uint64_t t, shared_ptr<boost::asio::steady_timer> timer, T f, _BoundArgs&&... __args)
{
    //new timer
    if (timer == nullptr)
        timer = boost::make_shared<shared_ptr<boost::asio::steady_timer>::element_type>(io_service);

    timer->expires_from_now(std::chrono::milliseconds(t));
    timer->async_wait(std::bind(f, this, __args..., timer));
    poco_trace_f1(*log, "createTimer(%Lu)", t);
}

shared_ptr<Guild> Bot::loadGuild(json & obj)
{
    shared_ptr<Guild> guild;

    //uint64_t application_id = obj->get("application_id").convert<uint64_t>();
    uint64_t id = obj["id"];

    try
    {
        guild = guildlist[id];
        if (guild == nullptr)
        {
            //not cached - create
            guildlist[id] = guild = boost::make_shared<Guild>(*this);
            guild->id = id;
            poco_trace_f1(*log, "Guild created: %Lu", id);
        }

#define GET_NULL(x,y) (x[y].is_null())?"":x[y]
        guild->name = GET_NULL(obj, "name");
        guild->icon = GET_NULL(obj, "icon");
        guild->splash = GET_NULL(obj, "splash");
        guild->owner_id = obj["owner_id"];
        guild->region = obj["region"];
        guild->afk_channel_id = GET_NULL(obj, "afk_channel_id");
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
        guild->voice_states = obj["voice_states"];
        json members = obj["members"];
        json channels = obj["channels"];
        json presences = obj["presences"];
        json roles = obj["roles"];
        json emojis = obj["emojis"];
        json features = obj["features"];

        for (auto & channel : channels)
        {
            uint64_t channel_id = channel["id"];

            try
            {
                //does channel exist?
                auto checkchannel = channellist[channel_id];
                if (checkchannel == nullptr)
                {
                    //not cached - create
                    channellist[channel_id] = checkchannel = boost::make_shared<Channel>();
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
                    checkchannel->last_message_id = (channel["last_message_id"].is_null()) ? 0 : (uint64_t)channel["last_message_id"];
                }


                json permission_overwrites = channel["permission_overwrites"];
                for (auto & permission : permission_overwrites)
                {
                    uint32_t allow = permission["allow"];
                    uint32_t deny = permission["deny"];
                    uint64_t p_id = permission["id"];
                    string p_role = GET_NULL(permission, "role");
                    //TODO: implement
                }

                guild->channellist.insert(std::pair<uint64_t, shared_ptr<Channel>>(checkchannel->id, checkchannel));
                channellist.insert(std::pair<uint64_t, shared_ptr<Channel>>(checkchannel->id, checkchannel));
            }
            catch (std::exception&e)
            {
                poco_error_f2(*log, "Error processing channel[%Lu] of guild[%Lu]", channel_id, id);
            }
        }

        for (auto & member : members)
        {
            json user = member["user"];
            uint64_t member_id = user["id"];
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

                checkmember->avatar = GET_NULL(user, "avatar");
                checkmember->discriminator = user["discriminator"];
                checkmember->name = GET_NULL(user, "name");

                checkmember->deaf = member["deaf"];
                checkmember->joined_at = member["joined_at"];
                checkmember->mute = member["mute"];
                checkmember->nick = GET_NULL(member, "nick");

                json roles = member["roles"];
                for (auto & r : roles)
                    checkmember->roles.push_back(r.get<uint64_t>());




            }
            catch (std::exception&e)
            {
                poco_error_f2(*log, "Error processing member[%Lu] of guild[%Lu]", member_id, id);
            }
        }

        for (auto  &role : roles)
        {
            shared_ptr<Role> _role = boost::make_shared<Role>();
            uint64_t role_id = role["id"];
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
                poco_error_f2(*log, "Error processing role[%Lu] of guild[%Lu]", role_id, id);
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
    }
    catch(std::exception&e)
    {
        poco_error_f1(*log, "Error processing guild[%Lu]", id);
    }

    return guild;
}
