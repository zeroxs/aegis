[![Build Status](https://travis-ci.org/zeroxs/aegis.svg?branch=master)](https://travis-ci.org/zeroxs/aegis) [![Discord](https://discordapp.com/api/guilds/287048029524066334/widget.png)](https://discord.gg/w7Y3Bb8) [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/zeroxs/aegis/blob/master/LICENSE)


AegisBot
=======

C++14 implementation of the [Discord API](https://discordapp.com/developers/docs/intro)

# License #

This project is licensed under the MIT license. See [LICENSE](https://github.com/zeroxs/aegis/blob/master/LICENSE)

# Installation instructions #
This is just the public repository of the AegisBot. If you'd like an instance in your server,
an easy link can be found [here](https://discordapp.com/oauth2/authorize?client_id=288063163729969152&scope=bot&permissions=2146958463) .
Please note, this link gives the bot full admin. Please refine it further if you wish to limit its abilities.



Libraries used:
- [Boost](http://www.boost.org) (log, iostreams, thread, system)
- [Websocketpp](https://github.com/zaphoyd/websocketpp)
- [PocoProject](https://github.com/pocoproject/poco) (Foundation, Net, NetSSL)
- [JSON for Modern C++](https://github.com/nlohmann/json)
- [OpenSSL](https://www.openssl.org)
- [zlib](https://zlib.net)

Optional Libraries:
- [RedisClient](https://github.com/nekipelov/redisclient) (used for caching and settings. not mandatory, but useful if you wish to have a non-hardcoded solution to configurations)

