[![Build Status](https://travis-ci.org/zeroxs/aegis.svg?branch=master)](https://travis-ci.org/zeroxs/aegis) [![Discord](https://discordapp.com/api/guilds/287048029524066334/widget.png)](https://discord.gg/w7Y3Bb8) [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/zeroxs/aegis/blob/master/LICENSE)


AegisBot
=======

C++14 implementation of the [Discord API](https://discordapp.com/developers/docs/intro)

This project started as an attempt to create a powerful library for the Discord API in C++ but has evolved into a fully-fledged bot framework. Some time in the future I will set up another repo with a heavily trimmed version that is much lighter and closer to the API.

# License #

This project is licensed under the MIT license. See [LICENSE](https://github.com/zeroxs/aegis/blob/master/LICENSE)

# Installation instructions #
This is just the public repository of the AegisBot. If you'd like an instance in your server,
an easy link can be found [here](https://discordapp.com/oauth2/authorize?client_id=288063163729969152&scope=bot&permissions=2146958463) .
Please note, this link gives the bot full admin. Please refine it further if you wish to limit its abilities.



Libraries used:
- [Boost 1.63+](http://www.boost.org) (log, iostreams, thread, system)
- [Websocketpp](https://github.com/zaphoyd/websocketpp)
- [PocoProject 1.7.7+](https://github.com/pocoproject/poco) (Foundation, Net, NetSSL)
- [JSON for Modern C++](https://github.com/nlohmann/json)
- [OpenSSL](https://www.openssl.org)
- [zlib](https://zlib.net)

Optional Libraries:
- [RedisClient](https://github.com/nekipelov/redisclient) (used for caching and settings. not mandatory, but useful if you wish to have a non-hardcoded solution to configurations)


# TODO #
- In-depth Linux set up guide
- In-depth Windows set up guide
- Voice data send/recv
- Partial rewrite for better horizontal scaling (split apart some pieces)
- Add hot-loadable module support
- Remove much internal caching and make use of the built-in solution or Redis
- Replace Boost.Log with [spdlog](https://github.com/gabime/spdlog)
- Replace PocoProject with cURL or [Boost.Beast](https://github.com/boostorg/beast)


# Building this library #
This guide is a WIP.
This source builds on both Windows and Linux.
Do note, even though this library will build and run on Windows, this library is designed to be run on Linux. Windows support is provided only for testing purposes.

Linux building is fairly straightforward and just needs the dependencies installed for the most part, run cmake, and then make. Proper Linux guide to come. (Note: If building Poco from source, it will need the Boost.Program_options library as well) For now, you can run the `deps-installer.sh` bash script which will completely obtain and build all dependencies for Ubuntu-based systems. Tested on a 16.04 and 17.04 fresh Ubuntu-server install.

Windows building is a fairly nasty beast and requires a bit of effort. I highly recommend building libraries from source properly. This list of downloads is for those that understand the risk of using 3rd-party pre-built binaries. Using these pre-built Poco and Boost libraries will require at minimum Visual Studio 14.0.
- [OpenSSL](https://slproweb.com/products/Win32OpenSSL.html) ([windows-headers](https://mega.nz/#!LJgiiQxD!VcU4dFs9ly0MhAxFCIWMMDJ0qR7nZhEppDb6_TOxhVw))
- [Poco 1.7.8-all](https://mega.nz/#!2AwEiLLI!YUt7EDJWInvh7wgVXctDOmdS1uHVf4U0xTFRZ4AnK1E)
- [Boost 1.63.0](https://mega.nz/#!TExCjYIb!cnaLe1ppgLCerto4RpdO28Qi5Ct8IE8Hik_6b3JJwtU)
- Libfmt will need to be built manually. (CMake -> VS -> Build)



# Using this library #
Focus here will be put on editing the config.json file.

Note: These configs are loaded prior to Redis so if you use Redis it will override anything you set here. This file is for default/firstrun settings. You can also configure all of this through Redis exclusively.<br />Starting the bot with the command line parameter `-overwritecache` will make the `config.json` values write to the Redis database.

The first step will be to put your bot's token in. If using Redis, you would set the key `config:token` to your token. If using the MemoryCache class instead, inside `config.json` you would set `token` to your bot's token.<br />
The `owner` field is your own `client_id` so that the bot identifies you as having full permissions. `master_server` and `master_channel` are the server and channel that the bot reports specific events such as `GUILD_CREATE` and any errors.<br />
`log-verbosity-file` and `log-verbosity-console` are the varying levels of verbosity for each. Options are `trace`, `debug`, `normal`, `warning`, `error`, `critical`. Default for both is `normal`.<br />
`default_commands` is the list of commands that are automatically added to any newly joined server. Admin-level commands cannot be added to this list.<br />
`default_servers` is an easy way to configure a set of servers. Within this block, `name` is an ignored field that is just to identify in the config. `id` is the `server_id` of the server. `defaultcmds` will specify if you want the `default_commands` to be added to this server. `cmdlist` will add the given commands to the server on top of any defaults you may have specified. Admin-level commands are only usable by the `owner` but can be accessed from any server.
