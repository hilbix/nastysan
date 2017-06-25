**State: Not ready to be used yet**

# NAStySAN

Re-implementation of sshfs in native Python for Linux FUSE.

Needs python modules paramiko and llfuse.

## Usage

    sudo apt-get install python3 python3-paramiko python3-llfuse
    git clone https://github.com/hilbix/nastysan.git
    cd nastysan
    ./nastysan arg

This is processed as

    mount $ARG
    host $ARG

like shown in the FUTURE COMMANDS below.


## Rants

For a long time now I wanted to have something, which is a NAS type SAN.  I never came around to create that beast.

This here is the first step forward.  It is a robust re-implementation of sshfs.  (Robust, because at my side, sshfs crashes for unknown reason.  This here gives me the opportunity to understand, what's going on.)

You might ask: Why reinvent the wheel?

This here compares to `sshfs` like a pneumatic wheel compares to a wooden wheel.  It offers higher speed on certain roads, is more comfortable and normally much more durable.

Current goal reached:

- None.  First goal is to be able to replace sshfs.


## License

This Works is placed under the terms of the Copyright Less License,
see file COPYRIGHT.CLL.  USE AT OWN RISK, ABSOLUTELY NO WARRANTY.

Read:  This is free as in free beer, free speech and free baby.

Read:  This code never can be copyrighted again.  Copyright is slavery and slavery must be abolished at all cost.  Or is here anybody who really wants to see a copyright on babies?


## FAQ

Q: Name?

- NAS stands for NAS
- ty stands for type
- SAN stands for SAN

Please note that the domains nastysan.com/net/org/de are owned by me since 2001 already.


Q: Contribute?

- Welcome.
- Send Pull requests on GitHub.  If you send too frequently, I add you as dev.  You have been warned.
- Waive copyright before sending pull requests.  Everything you send to me must be already covered by CLL.
- Try to keep the style.  Try to stay pythonic.


Q: External libs?

- All must be FLOSS.
- If they are not part of Debian, they must become part of this as `git submodule`.


Q: Python Module?

- I do not need it for me yet, but I will support it if someone does it.
- If you want to transform it according to the spec, please send pull request.


Q: Debian packaging?

- If it has matured, I probably will add `gbp` to this to make it easy to dreate a Debian native package out of this.
- However I do not want to become a Debian maintainer:
  - I hate politics because it corrupts people because you cannot stay authentic while acting politically correct.  But there is a fundamental principle: Without authenticity there cannot be trust.
  - With the decision against Init Freedom, Debian started to become a high risk political system like Wikipedia.  I am not willing to become a part of such a beast, ever.


Q: Bugs?

- Please open an issue on GitHub.  Be prepared to wait long.  I never have time to spare.


Q: CLL?

- You are free to drop this weird CLL, but then nothing informs you, that adding a Copyright breaks the law.


# FUTURE

`./nastysan [-|command-file.nastysan]..`

If the command-file is missing, options are read from stdin.

All the commands are taken from a file, because it is far too complicated to define everything on commandline.

The commands are line-by-line in the form `command args..`.


## COMMANDS

Following commands will be implemented first in future:

- `auto ARG` is exactly the same of what you can give on the commandline.
  - It only supports a single argument.
  - You can use `-` for stdin.  (Use `./-` to access a file named `-`)
  - You can use a number for the given unix file descriptor.  (Use `./#` to access a file with that name.)
  - If the given argument is a directory, `.nastysan` is appended.  Note that this is done literally, so `/a` gives `/a.nastysan` while `/a/` gives `/a/.nastysan`.
  - If the file is missing, it is treated as if following was specified (where `$ARG` denotes the given argument):

        config $HOME/.nastysan/$ARG.nastysan
        mount $ARG
        host $ARG

    Please note that `nastysan /mnt/somedrive` reads following entry from `~/.ssh/config`, which is completely valid syntax and works as expected:

        Host /mnt/somedrive
        Hostname samba.server.example.com
        User exampleuser

- `mount /path/to/mountpoint` starts the definition of a mount.  You can give more than one definition, all following commands refer to the latest definition.
  - If the argument is absent or the `mount` command is missing, it defaults to the name of the current processed commandline/`auto`/`conf`/`must` `ARG`.
  - If the argument is just a name without a path, `$HOME/mnt/` is prepended (to use a relative path, use `./file`).
  - The argument is transformed in an absolute path with all softlinks resolved.
  - If the path is not a directory, the extension `.nastysan` is removed.
  - If this is not a directory either (or has no file part), `.dir` is appended (so with stdin, the default path is `./.dir`).
  - The result must be an existing directory, else the command fails.
  - Mounts are read/write if there is any read-write host connection defined.

- `cache meta SEC` caches metadata responses for `SEC` seconds, default is 10000.
- `cache ram MB` use the given size of memory as a data cache, default is 100.
- `cache dir PATH` defines a directory which is used for caching.  It should be on a very fast (SSD) device.  The full free space of the device is used.

- `host SFTP` opens an SFTP connection to the given host.  The argument is resolved as in the `sftp` command.

- `path PATH` sets the path part of the destination.  Instead of `host host:path` you can give `path path` and `host host`.


Following commands will be implemented later:

- `conf ARG`, as `auto`, but there is no default action in case it is missing, in that case it is silently ignored.

- `must ARG`, as `conf`, but the file must exist.  Also stdin is not supported.  (An open unix file descriptor can only be used once.)

- `prefix DIR` sets the default `DIR` part for the `host` command.  If `DIR` is absent it is interpreted as `/` (which is equivalent to the missing path).

- `host SFTP [DIR]` opens an SFTP connection to the given host.  The argument is resolved as in the `sftp` command.
  - `DIR` gives the path, relative to the mountpoint, where the given connection shows up.
  - NOT YET IMPLEMENTED You can give multiple hosts.  In that case the filesystems are overlaied.  More about unions see below.

- `readonly BOOL` sets readonly mode.  This means, all connections to the following `host` commands are never used for write operations.
- `readwrite BOOL` is just the opposite of `readonly`.

- `timeout ARG` timeout for network operations.  If the destination does not answer in the given time, the operation is aborted and retried.
- `retry ARG` defines the number of retries until giving up.  Defaults to `-1` which means unlimited retries.  `retry 0` never does retries.
- `error ARG` define grace period, which means, after the given time retries are done again.  Defaults to `-1` which means, permanently fail.  `error 0` immediately has grace, with `retry 0` this means to do the retry after the request.
- `fail BOOL` makes the mountpoint fail in case of a timeout or other failure.  This is the default.  There are following special cases:
  - `fail false` means, that everything is handled as in the `fail true` case, but the next possible destination is considered.
  - For `fail true`:
    - `retry 0  error -1`: The failure is permanent as the mount is dead.  `nastysan` exits with error code if there are no more operable mounts.
    - `retry 0  error 0 `: Retry after the request.  So it fails fast.
    - `retry -1 error 1 `: Do retries (blocking).  Then, all other (and parallel) requests in the next second will immediately fail, until the next retry is done.
    - `retry -1 error 2 `: Do retries (blocking).  Then, all other (and parallel) requests in the next two seconds will immediately fail, until the next retry is done.  And so on.
    - `retry -1 error 0 `: Failure is reported after the first unsuccessful retry.  This is per request.
    - `retry -2 error 0 `: Failure is reported after the second unsuccessful retry.  And so on.
    - `retry -1 error -1`: Block until the connection is up again.
    - `retry 1  error 0 `: Fail all requests which piled up if a retry is unsuccessful.
    - `retry 1  error -1`: Each request does a retry.

- `multi BOOL` defines that the following `host`s (until the next `multi` definition) are alternative paths, so the same data can be reached over all those links.
  - This modifies `retry` such, that the first open alternative path is taken.
  - Retries of failing paths is done in the background.
  - In the `error 0` case, each request immediately triggers a single retry.
  - `error N` is the same, but this retry is delayed `N` seconds.
  - `error -N` means, the first request triggers a full retry cycle in the background, each `N` seconds apart.
  - In case that all alternative paths are down, failure is reported normally.
- `weight ARG` define the weight of the `multi` path.  Default is `-1` which means "no weight", which uses the first open path.
  - Destinations which have the same weight are used equally often.
  - `weight 0` means, do not use, this is purely a fallback.
  - `weight -1` always uses this path.  You can give `weight -2` which is preferred over `weigth -1` in case the definations must be used in a different order than given.
  - If two `weigth 50` and one `weight -1` are open, the `-1` wins.
  - `readonly` is supported, so writes go only to destinations which are `readwrite`.

- `first DIR` the first destination wins, which contains the directory.  This is the default.  On writes, the first destination must be writeable.
- `last DIR` the last writeable destination wins.  If none is writeable, the last one wins.  Note that this always tries to open files read/write.
- `union DIR` allows, to merge directories.  In that case, all destinations must be searched for existence of the directory.
  - In a union the file's attribute defines, which destination wins.
  - In a degenerated case, where some destination has incorrect timestamps, timestamps are manipulated such, that the correct one wins.  However this can have a race condition, so beware.
- `single DIR` is the opposite as `union`.  In that case it is an error if a file is available on more than one destination.


## Special args

- `BOOL` is a truish/falsish value and always defaults to `true`.  `0`, `n`, `no`, `off`, `false` is false, while `1`, `y`, `yes`, `on`, `true` is true.  All other values are errors.
  - If `BOOL` is used in a command execution context where some external command provides the result, the result code of the command is interpeted the `sh` way, so `0` is `true`, while anything else is `false`.

- `SFTP` is anything which is understood by `sftp`, too.  The format is `hostpart:filepart`

## Goals

These goals might be reached in future some time, but not neccessary in this sequence:

- Dynamical:  Change configuration on the fly, no need to umount.

- Distributed:  Mounts can be distributed across several different hosts.

- Multipath:  Paths can be reachable via different links in parallel.  Writes can go to different links than reads.

- Aggregates:  Mounts can have more than one target.

- Reliable:  If things break (network outage) it should be able to behave like NFS.  For example, if you reboot the `ssh` server, nothing shall break.

- Lowlevel:  NAStySAN currently uses ssh as a transport.  However some more low-level transports shall be there in the future.

- Hierarchical:  Allow to store things on different levels with transparent online migration scheme of data.

- Integer:  Validate data

- Shared:  Allow more than one host to access the filesystem in parallel.

- Transactional:  Synchronize hosts on the filesystem level.  (Shared locks etc.)

- Cached:  Continue to work even if offline.

- Redundant:  Implement n-type redundancy, where n links out of m can fail and still all data can be recovered.

- Robust:  Detect and fix silent errors

- Integrated Snapshots and Backups

Some of those are SAN features, some of those are NAS features.  Combine them both and you have NAStySAN.

It would be nice if it had volume support, such that you can create additional volumes without the need of things like the loop filesystem.

(The very first idea of NAStySAN was the way round, based on a lowlevel driver which offers just a block device.  This has changed.)

