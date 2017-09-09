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


# Building this source #
This guide is a WIP.
This source builds on both Windows and Linux.
Linux building is fairly straightforward and just needs the dependencies installed for the most part, run cmake, and then make. Proper Linux guide to come. (Note: If building Poco from source, it will need the boost::program_options library as well)

Windows building is a fairly nasty beast and requires a bit of effort. I highly recommend building libraries from source properly. This list of downloads is for those that understand the risk of using 3rd-party pre-built binaries.
- [OpenSSL](https://slproweb.com/products/Win32OpenSSL.html) ([windows-headers](https://mega.nz/#!LJgiiQxD!VcU4dFs9ly0MhAxFCIWMMDJ0qR7nZhEppDb6_TOxhVw))
- [Poco 1.7.8-all](https://mega.nz/#!2AwEiLLI!YUt7EDJWInvh7wgVXctDOmdS1uHVf4U0xTFRZ4AnK1E)
- [Boost 1.63.0](https://mega.nz/#!TExCjYIb!cnaLe1ppgLCerto4RpdO28Qi5Ct8IE8Hik_6b3JJwtU)
Libfmt will need to be built manually. (CMake -> VS -> Build)



