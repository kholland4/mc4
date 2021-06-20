# Deploying the server

The server binary, `mc4-server` is just a monolithic executable.
You can deploy it pretty much however you want.
That said, there are a couple data files to be aware of, and I've found it
useful to create a systemd unit file, build helper scripts, etc. to aid in the
process.

## TL;DR

* Compile with helper script: `./build-generic-x86_64.sh -C .. -j4`
  * Look at the [README](../../README.md) for a list of dependencies to install
* Copy `mc4-server`, `defs.json`, and `config_example.ini` to your target
* Configure TLS parameters in `config_example.ini`
* Modify `mc4-server.service` to fit your system and `systemctl enable` it
* `systemctl start` to get going!

## Build

Build using the Makefile in the `server/` directory.
For deployment, using TLS is strongly recommended.
If you need a certificate, get one from
[Let's Encrypt](https://letsencrypt.org/getting-started/)
([instructions](https://certbot.eff.org/lets-encrypt/othersnap-other.html)).
If for some reason you aren't using TLS, be sure to remove the `-DTLS` option
from any build scripts you use.

Build scripts are provided for several target architectures (`build-*.sh`).
They do nothing more than set `$CXX` and `$CPPFLAGS` to optimize for the target
(and set `-DTLS` of course!).
These scripts run `make` and pass along all arguments.
For example:

    $ ./build-aws-graviton2.sh -C .. -j4
    $ cp ../mc4-server /path/to/target/mc4-server
    $ cp ../defs.json /path/to/target/defs.json
    $ cp ../config_example.ini /path/to/target/config.ini

(The `-C` flag tells `make` where to find the Makefile and source code.)

## Files

In addition to the `mc4-server` binary, the server uses some data and config
files.
These are:

### defs.json

Node, item, and craft definitions.
By default, the server looks for this in its current directory.
The path to this file can be configured.

### config_example.ini

Config file, which should be named `config.ini` or something more appropriate
once you've put your options in it.
`mc4-server` must be told where to find this file:

    $ ./mc4-server --config-file config.ini

### test_map.sqlite

SQLite database containing map, player, and any other server state.
The name and location of this file can be configured.

### TLS certificate

See [config_example.ini](../config_example.ini) for an explanation of required
TLS options.

## Configuration

The server can be configured using an ini file
(see [config_example.ini](../config_example.ini)) or on the command line.
For example:

    $ ./mc4-server --config-file config_example.ini
    $ ./mc4-server -o server.port=8081
    $ ./mc4-server --config-file config_example.ini -o "server.motd=-- Welcome to the server.\n-- Have a nice day."

Note that command line options are processed in the order given,
so if you want to override an option from the config file,
put your override *after* the config file.

By default, the server will run on port `8080` and store data to an SQLite database in `test_map.sqlite`.

    $ ./mc4-server

## Installation

While you can run the server directly from a command shell, in a screen session,
etc., it is generally desirable to run it as a service.
A systemd unit file is provided for this purpose:
[mc4-server.service](mc4-server.service).
Edit this file to point to your binary and config files, customize it further to
fit your system setup, then copy it to
`/etc/systemd/system/`:

    # cp mc4-server.service /etc/systemd/system/
    # nano /etc/systemd/system/mc4-server.service
    # systemctl enable mc4-server
    # systemctl start mc4-server
    # systemctl status mc4-server

## Connect!

Hop on the web client and put in the address of your server, something like

    wss://host.example.com:8080/

depending on configuration.
You can connect as guest, register an account, etc. -- the game is ready to go!

### Privileges

Players have privileges that determine what they may do in game, such as `fast`,
`fly`, `creative`, or `teleport`.
By default, all guests and newly-registered players get `interact`, `shout`, and
`touch`; these allow basic interaction with the game and can be revoked as a
means of moderation.
You must have the `grant` privilege to grant most privileges (or `grant_basic`
for some of the lower-level ones).
There's one exception: the `grant` privilege itself and the omnipotent `admin`
privilege require the `admin` privilege to grant.
If you register an account on your new server, there's no way to get that
privilege.
The current workaround to solve that is to set `player.default_grants="admin interact shout touch"`
temporarily, register your account, then unset it.

    $ sudo systemctl stop mc4-server
    $ ./mc4-server --config-file config.ini -o player.default_grants="admin interact shout touch"
    (register your new account)
    (Ctrl-C)
    $ sudo systemctl start mc4-server
