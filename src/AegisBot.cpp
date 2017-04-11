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

#include <fstream>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/sinks/unbounded_ordering_queue.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/record_ordering.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/keywords/file_name.hpp>
#include <boost/log/keywords/rotation_size.hpp>
#include <boost/log/keywords/time_based_rotation.hpp>
#include <boost/log/keywords/target.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/core/null_deleter.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

#ifdef USE_REDIS
#include "ABRedisCache.h"
#endif

template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, severity_level level)
{
    static const char* const str[] =
    {
        "trace",
        "debug",
        "normal",
        "warning",
        "error",
        "critical"
    };
    if (static_cast<std::size_t>(level) < (sizeof(str) / sizeof(*str)))
        strm << str[level];
    else
        strm << static_cast<int>(level);
    return strm;
}

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

using boost::shared_ptr;

//#include <curl/curl.h>

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", uint64_t)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(tag_attr, "Tag", std::string)

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
std::map<uint64_t, Member*> AegisBot::memberlist;
std::map<uint64_t, Guild*> AegisBot::guildlist;
std::vector<AegisBot*> AegisBot::shards;
boost::asio::io_service AegisBot::io_service;
uint16_t AegisBot::shardidmax;
string AegisBot::mention;
string AegisBot::tokenstr;
//std::map<string, <>> AegisBot::baseModules;
std::map<string, uint64_t> AegisBot::eventCount;

AegisBot::AegisBot()
    : keepalive_timer_(io_service)
    , prunemessages(io_service)
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
    for (auto g : memberlist)
    {
        delete g.second;
    }
    for (auto g : channellist)
    {
        delete g.second;
    }
    for (auto s : shards)
    {
        delete s;
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
    if (AegisBot::memberlist.count(id))
        return *AegisBot::memberlist[id];
    //AegisBot::globalusers[id] = Member();
    AegisBot::memberlist.insert(std::pair<uint64_t, Member*>(id, new Member()));
    AegisBot::memberlist[id]->id = id;
    return *AegisBot::memberlist[id];
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
    throw std::domain_error("Exception: Guild does not exist");
}

Member & AegisBot::getMember(uint64_t id)
{
    if (AegisBot::memberlist.count(id))
        return *AegisBot::memberlist[id];
    throw std::domain_error("Exception: Member does not exist");
}

Channel & AegisBot::getChannel(uint64_t id)
{
    if (AegisBot::channellist.count(id))
        return *AegisBot::channellist[id];
    throw std::domain_error("Exception: Channel does not exist");
}

void AegisBot::setupCache(ABCache * in)
{
    cache = in;
}

bool AegisBot::initialize(uint64_t shardid)
{
    this->shardid = shardid;

    eventCount =
    {
        { "TYPING_START", 0 },
        { "MESSAGE_CREATE", 0 },
        { "MESSAGE_UPDATE", 0 },
        { "GUILD_CREATE", 0 },
        { "GUILD_UPDATE", 0 },
        { "GUILD_DELETE", 0 },
        { "MESSAGE_DELETE", 0 },
        { "MESSAGE_DELETE_BULK", 0 },
        { "USER_SETTINGS_UPDATE", 0 },
        { "USER_UPDATE", 0 },
        { "VOICE_STATE_UPDATE", 0 },
        { "READY", 0 },
        { "CHANNEL_CREATE", 0 },
        { "CHANNEL_UPDATE", 0 },
        { "CHANNEL_DELETE", 0 },
        { "GUILD_BAN_ADD", 0 },
        { "GUILD_BAN_REMOVE", 0 },
        { "GUILD_EMOJIS_UPDATE", 0 },
        { "GUILD_INTEGRATIONS_UPDATE", 0 },
        { "GUILD_MEMBER_ADD", 0 },
        { "GUILD_MEMBER_REMOVE", 0 },
        { "GUILD_MEMBER_UPDATE", 0 },
        { "GUILD_MEMBER_CHUNK", 0 },
        { "GUILD_ROLE_CREATE", 0 },
        { "GUILD_ROLE_UPDATE", 0 },
        { "GUILD_ROLE_DELETE", 0 },
        { "PRESENCE_UPDATE", 0 },
        { "VOICE_SERVER_UPDATE", 0 }
    };

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
    log(fmt::format("Connecting to gateway at {0}", gatewayurl), severity_level::normal);
    connection = ws.get_connection(gatewayurl + "/?encoding=json&v=6", ec);
    if (ec)
    {
        log(fmt::format("Connection failed: {0}", ec.message()), severity_level::error);
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
    boost::log::sources::severity_logger<severity_level> slg;
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
    BOOST_LOG_SEV(slg, normal) << fmt::format("Shard count: {0}", ret["shards"].get<uint16_t>());
    shardidmax = ret["shards"];
#else
    shardidmax = 1;
#endif

    for (int i = 0; i < AegisBot::shardidmax; ++i)
    {
        AegisBot::shards.push_back(new AegisBot);
        AegisBot::shards[i]->initialize(i);
        AegisBot::threadPool.push_back(std::thread([=]()
        {
            if (i == 0)
            {
                //only have shard 0 do maint stuff since it's always guaranteed to be loaded
                AegisBot::shards[i]->prunemessages.expires_from_now(std::chrono::seconds(30));
                AegisBot::shards[i]->prunemessages.async_wait([=](const boost::system::error_code & ec) { AegisBot::shards[i]->pruneMsgHistory(ec); });
            }

            AegisBot::shards[i]->run();
        }));
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
    log("Connection closed. Reconnecting.", severity_level::warning);
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
        log(fmt::format("Guild created: {0}", guild.id), severity_level::trace);
        guild.unavailable = unavailable;
        if (!unavailable)
        {
            loadGuild(guildobj);

            {
                //temporary. This won't work when the bot is in over 120 guilds due to ratelimits over websocket
                json obj;
                obj["op"] = 8;
                obj["d"]["guild_id"] = id;
                obj["d"]["query"] = "";
                obj["d"]["limit"] = 0;
                wssend(obj.dump());
            }
        }
    }

/*
    json presences = d["presences"];
    for (auto & presenceobj : presences)
    {
        //TODO
    }*/

    private_channels.clear();
    json pchannels = d["private_channels"];
    for (auto & channel : pchannels)
    {
        uint64_t channel_id = std::stoull(channel["id"].get<string>());
        //uint64_t last_message_id = channel["last_message_id"].is_null()?0:std::stoull(channel["last_message_id"].get<string>());
        //int32_t channelType = channel["type"];
        json recipients = channel["recipients"];
      
        PrivateChat & privateChat = private_channels[channel_id];//test
        log(fmt::format("Private Channel created: {0}", channel_id), severity_level::trace);
        privateChat.id = channel_id;
        
        for (auto & recipient : recipients)
        {
            string recipientAvatar = recipient["avatar"].is_null()?"":recipient["avatar"];
            uint16_t recipientDiscriminator = std::stoi(recipient["discriminator"].get<string>());
            string recipientName = recipient["username"];
            uint64_t recipientId = std::stoull(recipient["id"].get<string>());

            //Member & rec = privateChat.recipients[recipientId];
            privateChat.recipients.push_back(recipientId);
            Member & glob = AegisBot::createMember(recipientId);
            log(fmt::format("Member created: {0}", channel_id), severity_level::trace);
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
        log(fmt::format("Member: {0}", AegisBot::mention), severity_level::normal);
    }

    username = userdata["username"];
    mfa_enabled = userdata["mfa_enabled"];
    active = true;
}

void AegisBot::onMessage(websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg)
{
    json result;
    string payload;
    try
    {
        //TODO: do something about this mess
        payload = msg->get_payload();

        if (payload[0] == (char)0x78 && (payload[1] == (char)0x01 || payload[1] == (char)0x9C || payload[1] == (char)0xDA))
        {
            boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
            std::stringstream origin(payload);
            in.push(boost::iostreams::zlib_decompressor());
            //in.push(boost::make_iterator_range(payload));
            in.push(origin);
            payload.clear();
            //boost::iostreams::copy(in, boost::iostreams::back_inserter(payload));
            std::stringstream ss;
            boost::iostreams::copy(in, ss);
            payload = ss.str();
        }
 
        result = json::parse(std::move(payload));

        //poco_trace_f1(*log, "Received JSON: %s", msg->get_payload());

        if (!result.is_null())
        {
            if (!result["t"].is_null())
            {
                string cmd = result["t"];
                //poco_trace_f1(*log, "Processing: %s", cmd);

                ++eventCount[cmd];

                if (cmd == "TYPING_START")
                {
                    //do we care? it'd easily be the most sent event
                    return;
                }
                else if (cmd == "MESSAGE_CREATE")
                {
                    while (!active && isrunning)
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    userMessage(result["d"]);
                }
                else if (cmd == "MESSAGE_UPDATE")
                {
                    //std::cout << result.dump() << std::endl;
                    json message = result["d"];
                    //uint64_t message_id = std::stoull(message["id"].get<string>());
                    //uint64_t channel_id = std::stoull(message["channel_id"].get<string>());

                    if (message["embeds"].size() > 0)
                    {
                        //parse embeds some time
/*
                        string type = message["type"];

                        string title;
                        if (message.count("title"))
                            title = message["title"];

                        string description;
                        if (message.count("description"))
                            description = message["description"];
                        uint32_t color;
                        if (message.count("color"))
                            color = message["color"];*/
                    }
                    else
                    {
/*
                        uint64_t user_id = std::stoull(message["author"]["id"].get<string>());
                        string content = message["content"];

                        string timestamp = message["timestamp"];
                        string edited_timestamp = message["edited_timestamp"];

                        bool mention_everyone = message["mention_everyone"];
                        bool pinned = message["pinned"];
                        //
                        json reactions;
                        if (message.count("reactions"))
                            reactions = message["reactions"];
                        json mentions = message["mentions"];
                        json mention_roles = message["mention_roles"];*/
                    }







                }
                else if (cmd == "GUILD_CREATE")
                {
                    loadGuild(result["d"]);

                    //load things like database commands and permissions here
                }
                else if (cmd == "GUILD_UPDATE")
                {
                    loadGuild(result["d"]);
                }
                else if (cmd == "GUILD_DELETE")
                {
                }
                else if (cmd == "MESSAGE_DELETE")
                {
                }
                else if (cmd == "MESSAGE_DELETE_BULK")
                {
                }
                else if (cmd == "USER_SETTINGS_UPDATE")
                {
                }
                else if (cmd == "USER_UPDATE")
                {
                }
                else if (cmd == "VOICE_STATE_UPDATE")
                {
                }
                else if (cmd == "READY")
                {
                    processReady(result["d"]);
                }



                //////////////////////////////////////////////////////////////////////////
                //start of guild_id events
                //everything beyond here has a guild_id

                //std::cout << result.dump() << std::endl;
                if (result["d"].count("guild_id"))
                {
                    Guild & guild = getGuild(std::stoull(result["d"]["guild_id"].get<string>()));
                    if (cmd == "CHANNEL_CREATE")
                    {
                        loadChannel(result["d"], guild.id);//untested
                    }
                    else if (cmd == "CHANNEL_UPDATE")
                    {
                    }
                    else if (cmd == "CHANNEL_DELETE")
                    {
                    }
                    else if (cmd == "GUILD_BAN_ADD")
                    {
                    }
                    else if (cmd == "GUILD_BAN_REMOVE")
                    {
                    }
                    else if (cmd == "GUILD_EMOJIS_UPDATE")
                    {
                    }
                    else if (cmd == "GUILD_INTEGRATIONS_UPDATE")
                    {
                    }
                    else if (cmd == "GUILD_MEMBER_ADD")
                    {
                        loadMember(result["d"], guild);//untested
                    }
                    else if (cmd == "GUILD_MEMBER_REMOVE")
                    {

                    }
                    else if (cmd == "GUILD_MEMBER_UPDATE")
                    {
                    }
                    else if (cmd == "GUILD_MEMBER_CHUNK")
                    {
                    }
                    else if (cmd == "GUILD_ROLE_CREATE")
                    {
                    }
                    else if (cmd == "GUILD_ROLE_UPDATE")
                    {
                    }
                    else if (cmd == "GUILD_ROLE_DELETE")
                    {
                    }
                    else if (cmd == "PRESENCE_UPDATE")
                    {
                        //std::cout << result.dump      () << std::endl;
                    }
                    else if (cmd == "VOICE_SERVER_UPDATE")
                    {
                    }
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
                            { "token", cache->get(tokenstr) },
                            { "properties",
                                {
                                    { "$os", "linux" },
                                    { "$browser", "aegis" },
                                    { "$device", "aegis" },
                                    { "$referrer", "" },
                                    { "$referring_domain", "" }
                                }
                            },
                            { "compress", true },
                            { "large_threshhold", 250 }
                        }
                    }
                };
                ws.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
            }
            if (result["op"] == 10)
            {
                uint64_t heartbeat = result["d"]["heartbeat_interval"];
                log(fmt::format("Heartbeat timer added : {0} ms.", heartbeat), severity_level::trace);
                keepalive_timer_.expires_from_now(std::chrono::milliseconds(heartbeat));
                keepalive_timer_.async_wait([heartbeat, this](const boost::system::error_code & ec) { keepAlive(ec, heartbeat); });
            }
            if (result["op"] == 11)
            {
                //heartbeat ACK
                log("Heartbeat ACK", severity_level::trace);
            }
        }
    }
//     catch (Poco::BadCastException& e)
//     {
//         BOOST_LOG_TRIVIAL(error) << "BadCastException: " << e.what();
//     }
    catch (no_permission & e)
    {
        AegisBot::channellist[std::stoull(result["d"]["channel_id"].get<string>())]->sendMessage(fmt::format("No permission: [{0}]", e.what()));
        log(fmt::format("No permission: [{0}]", e.what()), severity_level::warning);
    }
    catch (std::exception& e)
    {
        log(fmt::format("Failed to process object: {0}", e.what()), severity_level::error);
        log(msg->get_payload(), severity_level::error);
    }
    catch (...)
    {
        log("Failed to process object: Unknown error", severity_level::error);
    }
}

void AegisBot::userMessage(json & obj)
{
    json author = obj["author"];

    uint64_t userid = std::stoull(author["id"].get<string>());
    string username = author["username"];

    uint64_t channel_id = std::stoull(obj["channel_id"].get<string>());
    uint64_t id = std::stoull(obj["id"].get<string>());
    string content = obj["content"];

    //process chat

    //look for command start. this can be configurable per server
    //processing of the entire string won't occur for now unless
    //this matches
    //options can be added later to enable full scanning on a
    //per-channel basis for things like word filters etc



    //make a queue for messages?

    //keep a temporary store of the id
    //create user if doesn't exist
    auto & member = createMember(userid);
    {
        std::lock_guard<std::mutex> lock(Member::m);
        member.msghistory.push(id);
    }

    try
    {
        Channel & channel = getChannel(channel_id);

        channel.guild().processMessage(obj);

        return;
    }
    catch (std::out_of_range & e)
    {
        for (auto & dm : private_channels)
        {
            if (dm.second.id == channel_id)
            {

                boost::char_separator<char> sep{ " " };
                boost::tokenizer<boost::char_separator<char>> tok{ content, sep };


                return;
            }
        }
    }
}

void AegisBot::keepAlive(const boost::system::error_code& error, const uint64_t ms)
{
    if (error != boost::asio::error::operation_aborted)
    {
        json obj;
        obj["d"] = sequence;
        obj["op"] = 1;

        log(fmt::format("Sending Heartbeat: {0}", obj.dump()), severity_level::trace);
        ws.send(connection, obj.dump(), websocketpp::frame::opcode::text);

        log(fmt::format("Heartbeat timer added: {0} ms", ms), severity_level::trace);
        keepalive_timer_.expires_from_now(std::chrono::milliseconds(ms));
        keepalive_timer_.async_wait([ms, this](const boost::system::error_code & ec) { keepAlive(ec, ms); });
    }
}

void AegisBot::onConnect(websocketpp::connection_hdl hdl)
{
    this->hdl = hdl;
    log("Connection established.", severity_level::normal);

    json obj;

    if (sessionId.size() > 0)
    {
        //reconnect
        obj = {
            { "op", 6 },
            {
                "d",
                {
                    { "token", cache->get(tokenstr) },
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
                    { "token", cache->get(tokenstr) },
                    {
                        "properties",
                        {
                            { "$os", "linux" },
                            { "$browser", "aegis" },
                            { "$device", "aegis" },
                            { "$referrer", "" },
                            { "$referring_domain", "" }
                        }
                    },
                    { "compress", true },
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
        boost::log::sources::severity_logger<severity_level> slg;
        URI uri("https://discordapp.com/api/v6" + url);
        std::string path(uri.getPathAndQuery());

#ifdef WIN32
        Context::Ptr context = new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
        HTTPSClientSession session(uri.getHost(), uri.getPort(), context);
#else
        HTTPSClientSession session(uri.getHost(), uri.getPort());
#endif
        HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
#ifdef SELFBOT
        request.set("Authorization", cache->get(tokenstr));
#else
        request.set("Authorization", string("Bot ") + cache->get(tokenstr));
#endif
        request.set("User-Agent", "DiscordBot (https://github.com/zeroxs/aegisbot 0.1)");
        request.set("Content-Type", "application/json");


        request.setMethod(method);

        if (obj.length() > 0)
        {
            request.setContentLength(obj.length());


#ifdef DEBUG_OUTPUT
            //std::cout << "Sent JSON: " << obj << "\n";

            std::stringstream ss;
            request.write(ss);
            BOOST_LOG_SEV(slg, trace) << "Sent request: " << ss.str() << std::endl;
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




        BOOST_LOG_SEV(slg, trace) << "status: " << (int)status << " " << response.getReason() << std::endl;

#ifdef DEBUG_OUTPUT
        std::stringstream ss;
        response.write(ss);
        BOOST_LOG_SEV(slg, trace) << "Response: " << ss.str() << std::endl;
#endif
        string result;

        Poco::StreamCopier::copyToString(rs, result);
#ifdef DEBUG_OUTPUT
        BOOST_LOG_SEV(slg, trace) << "Result: " << result << "\n";
#endif

        if (endpoint)
        {
            endpoint->resetFailure();
            if (response.has("X-RateLimit-Global"))
            {
                rate_global = true;
                endpoint->rateRetry(Poco::DynamicAny(response.get("Retry-After")).convert<uint32_t>());
                BOOST_LOG_SEV(slg, error) << fmt::format("Global rate limit reached. Pausing for {0} ms", endpoint->rateRetry());
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
                BOOST_LOG_SEV(slg, trace) << fmt::format("Rates: {0}:{1} resets in: {2}s", endpoint->rateLimit(), endpoint->rateRemaining(), (endpoint->rateReset() > 0) ? (endpoint->rateReset() - epoch) : 0);
            }
        }

        return { true, result };
    }
    catch (std::exception&e)
    {
        BOOST_LOG_TRIVIAL(error) << "Unhandled error in Bot::call(): " << e.what();
        endpoint->addFailure();
    }


    return { false, "" };
}

void AegisBot::pruneMsgHistory(const boost::system::error_code& error)
{
    if (error != boost::asio::error::operation_aborted)
    {
        std::lock_guard<std::mutex> lock(Member::m);

        log("Starting message prune", severity_level::trace);
        //2 hour expiry
        uint64_t epoch = ((std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - (2 * 60 * 60 * 1000)) - 1420070400000) << 22;
        for (auto & member : memberlist)
        {
            if (member.second->msghistory.size() > 0 && member.second->msghistory.front() < epoch)
            {
                log(fmt::format("Removed message [{0}] from [{1}] time: {2}", member.second->msghistory.front(), member.second->id, epoch), severity_level::trace);
                member.second->msghistory.pop();
            }
        }

        prunemessages.expires_from_now(std::chrono::seconds(30));
        prunemessages.async_wait([this](const boost::system::error_code & ec) { pruneMsgHistory(ec); });
    }
}

void AegisBot::purgeMsgHistory()
{
    std::lock_guard<std::mutex> lock(Member::m);
    for (auto & member : memberlist)
    {
        member.second->msghistory = std::queue<uint64_t>();
    }
}

void AegisBot::run()
{
    while (isrunning)
    {




        // Check for outgoing messages that need sending
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
                                    log(fmt::format("Rate Limit hit or connection error - requeuing message [{0}] [{1}] [{2}] [{3}]", message.channel().guild().name, message.channel().guild().id, message.channel().name, message.channel().id), severity_level::error);
                                    continue;
                                }
                                //message success, pop
                                message.channel().ratelimits.outqueue.pop();
                            }

                            log(fmt::format("Message sent: [{0}] [{1}]", message.endpoint, message.content), severity_level::trace);
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
                                log(fmt::format("Rate Limit hit or connection error - requeuing message [{0}] [{1}] [{2}] [{3}]", message.channel().guild().name, message.channel().guild().id, message.channel().name, message.channel().id), severity_level::error);
                                continue;
                            }
                            guild.second->ratelimits.outqueue.pop();
                        }
                        log(fmt::format("Message sent: [{0}] [{1}]", message.endpoint, message.content), severity_level::trace);
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
    uint64_t id = std::stoull(obj["id"].get<string>());


    Guild & guild = createGuild(id);

    try
    {
        log(fmt::format("Guild created: {0}", id), severity_level::trace);
        guild.id = id;

        //guild->cmdlist = defaultcmdlist;

#define GET_NULL(x,y) (x[y].is_null())?"":x[y]
        guild.name = GET_NULL(obj, "name");
        guild.icon = GET_NULL(obj, "icon");
        guild.splash = GET_NULL(obj, "splash");
        guild.owner_id = std::stoull(obj["owner_id"].get<string>());
        guild.region = obj["region"];
        guild.afk_channel_id = obj["afk_channel_id"].is_null() ? 0 : std::stoull(obj["afk_channel_id"].get<string>());
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

        if (obj.count("roles"))
        {
            json roles = obj["roles"];

            for (auto & role : roles)
            {
                loadRole(role, guild);
            }
        }

        if (obj.count("members"))
        {
            json members = obj["members"];

            for (auto & member : members)
            {
                loadMember(member, guild);
            }
        }

        if (obj.count("channels"))
        {
            json channels = obj["channels"];

            for (auto & channel : channels)
            {
                loadChannel(channel, id);
            }
        }

        if (obj.count("presences"))
        {
            json presences = obj["presences"];

            for (auto & presence : presences)
            {
                loadPresence(presence, guild);
            }
        }

        if (obj.count("emojis"))
        {
            json emojis = obj["emojis"];

            for (auto & emoji : emojis)
            {
                loadEmoji(emoji, guild);
            }
        }

        if (obj.count("features"))
        {
            json features = obj["features"];

        }


        {
            uint64_t perms = 0;
            for (auto & r : guild.memberlist[userId].first->roles)
            {
                perms |= guild.rolelist[r].permissions;
            }
            guild.updatePerms(perms);
        }

        for (auto & c : guild.channellist)
        {
            uint64_t perms = 0;
            for (auto & r : guild.memberlist[userId].first->roles)
            {
                perms |= guild.rolelist[r].permissions;
            }
            c.second->updatePerms(perms);
        }





/*
        for (auto & feature : features)
        {
            //??
        }

        for (auto & voicestate : voice_states)
        {
            //no voice yet
        }*/



    }
    catch(std::exception&e)
    {
        log(fmt::format("Error processing guild[{0}] {1}", id, (string)e.what()), severity_level::error);
    }
}

void AegisBot::loadChannel(json & channel, uint64_t guild_id)
{
    uint64_t channel_id = std::stoull(channel["id"].get<string>());
    Guild & guild = getGuild(guild_id);

    try
    {
        //does channel exist?
        Channel & checkchannel = createChannel(channel_id, guild_id);
        log(fmt::format("Channel[{0}] created for guild[{1}]", channel_id, guild_id), severity_level::trace);
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
            checkchannel.last_message_id = (channel["last_message_id"].is_null()) ? 0 : std::stoull(channel["last_message_id"].get<string>());
        }


        json permission_overwrites = channel["permission_overwrites"];
        for (auto & permission : permission_overwrites)
        {
            uint32_t allow = permission["allow"];
            uint32_t deny = permission["deny"];
            uint64_t p_id = std::stoull(permission["id"].get<string>());
            string p_type = GET_NULL(permission, "type");

            if (p_type == "role")
            {
                guild.rolelist[p_id].overrides[channel_id].allow = allow;
                guild.rolelist[p_id].overrides[channel_id].deny = deny;
                guild.rolelist[p_id].overrides[channel_id].id = channel_id;
                guild.rolelist[p_id].overrides[channel_id].type = Override::ORType::ROLE;
            }
            else
            {
                guild.rolelist[p_id].overrides[channel_id].allow = allow;
                guild.rolelist[p_id].overrides[channel_id].deny = deny;
                guild.rolelist[p_id].overrides[channel_id].id = channel_id;
                guild.rolelist[p_id].overrides[channel_id].type = Override::ORType::USER;
            }
        }

        guild.channellist.insert(std::pair<uint64_t, Channel*>(checkchannel.id, &checkchannel));
    }
    catch (std::exception&e)
    {
        log(fmt::format("Error processing channel[{0}] of guild[{1}] {2}", channel_id, guild_id, e.what()), severity_level::error);
    }
}

void AegisBot::loadMember(json & member, Guild & guild)
{
    uint64_t guild_id = guild.id;

    json user = member["user"];
    uint64_t member_id = std::stoull(user["id"].get<string>());
    try
    {
        Member & checkmember = createMember(member_id);
        log(fmt::format("Member[{0}] created for guild[{1}]", member_id, guild_id), severity_level::trace);
        guild.memberlist[member_id] = std::pair<Member*, uint16_t>(&checkmember, 0);

        checkmember.avatar = GET_NULL(user, "avatar");
        checkmember.discriminator = std::stoi(user["discriminator"].get<string>());
        checkmember.name = GET_NULL(user, "username");

        checkmember.deaf = member["deaf"];
        checkmember.joined_at = member["joined_at"];
        checkmember.mute = member["mute"];
        checkmember.isbot = member["bot"].is_null() ? false : true;
        checkmember.guilds[guild_id].nickname = GET_NULL(member, "nick");

        json roles = member["roles"];
        for (auto & r : roles)
            checkmember.roles.push_back(std::stoull(r.get<string>()));

        checkmember.guilds[guild_id].guild = &getGuild(guild_id);
    }
    catch (std::exception&e)
    {
        log(fmt::format("Error processing member[{0}] of guild[{1}] {2}", member_id, guild_id, e.what()), severity_level::error);
    }
}

void AegisBot::loadRole(json & role, Guild & guild)
{
    uint64_t guild_id = guild.id;

    Role _role;
    uint64_t role_id = std::stoull(role["id"].get<string>());
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
        log(fmt::format("Error processing role[{0}] of guild[{1}] {2}", role_id, guild_id, e.what()), severity_level::error);
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

void AegisBot::setupLogging()
{
    boost::shared_ptr< logging::core > core = logging::core::get();
    logging::add_common_attributes();


    attrs::local_clock TimeStamp;
    logging::core::get()->add_global_attribute("TimeStamp", TimeStamp);

    {
        boost::shared_ptr< sinks::text_file_backend > backend =
            boost::make_shared< sinks::text_file_backend >(
                keywords::file_name = "aegis_%Y%m%d.log",
                keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0)
                );

        backend->auto_flush(false);

        typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
        boost::shared_ptr< sink_t > sink(new sink_t(backend));

        sink->set_filter(severity >= normal);

        logging::formatter fmt = expr::stream
            << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S") << " : "
            << "<" << severity << ">\t"
            << expr::smessage;

        sink->set_formatter(fmt);
        core->add_sink(sink);
    }

    {
        boost::shared_ptr< sinks::text_ostream_backend > backend = boost::make_shared< sinks::text_ostream_backend >();
        backend->add_stream(boost::shared_ptr< std::ostream >(&std::cout, boost::null_deleter()));

        backend->auto_flush(false);


        typedef sinks::synchronous_sink< sinks::text_ostream_backend > sink_t;
        boost::shared_ptr< sink_t > sink(new sink_t(backend));

        sink->set_filter(severity >= debug);


        logging::formatter fmt = expr::stream
            << "[" << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S") << "] : "
            << "<" << std::setw(8) << severity << ">\t"
            << expr::smessage;

        sink->set_formatter(fmt);
        core->add_sink(sink);
    }
}

void AegisBot::log(string message, severity_level level)
{
    boost::log::record rec = slg.open_record(keywords::severity = level);
    if (rec)
    {
        logging::record_ostream strm(rec);
        strm << message;
        strm.flush();
        slg.push_record(boost::move(rec));
    }
}

uint64_t AegisBot::convertDateToInt64(std::string timestamp)
{
    boost::posix_time::ptime pt;
    std::stringstream ss;
    ss.imbue(std::locale(std::locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%S")));
    ss << timestamp;
    ss >> pt;
    if (pt != boost::posix_time::ptime())
    {
        boost::posix_time::ptime timet_start(boost::gregorian::date(1970, 1, 1));
        boost::posix_time::time_duration diff = pt - timet_start;
        return diff.ticks() / boost::posix_time::time_duration::rep_type::ticks_per_second;
    }
    return 0;
}

