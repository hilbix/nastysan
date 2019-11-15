> This is in the PRE-PLANNING phase
>
> **State: Not ready to be used yet**

# NAStySAN

FUSE base layer for reliable redundant block devices.


## Usage

    sudo apt-get install python3 python3-paramiko python3-llfuse
    git clone https://github.com/hilbix/nastysan.git
    cd nastysan
    ./nastysan.py [OPTIONS] CONFIG MOUNTPOINT

or with `/etc/fstab`:

	nastysan.py#CONFIG	mountpoint	fuse	user,OPTIONS	0 0

You can also do:

	ln nastysan.py /usr/bin/mount.nastysan

and use following entry in `/etc/fstab`:

	CONFIG	mountpoint	nastysan	user,OPTIONS	0 0


## CONFIG

Usually, CONFIG is just a writeable file.

When the config file starts exists, starts with a SheBang `#!` or is executable,
the argument is executed by NAStySAN with following properties:

- To read the configuration, the script is called without parameters.
  - The script must output the configuration to STDOUT
- To write the configuration, the script is called with a file as parameter.
  - The script then must compare STDIN with the contents of the file contents.
  - If equal, the contents of the configuration must be replaced.
  - Success is returned by error code 0, everything else is a failure.

Example such script `/etc/nastysan.conf`:

	#!/bin/bash

	CONF=/real/config.dat
	[ 0 = "$#" ] && exec cat "$CONF"

	cat > "$CONF.tmp" &&
	sync &&
	cmp "$CONF.tmp" "$1" &&
	mv -f "$CONF.tmp" "$CONF"

If NAStySAN was installed to `/usr/local/` this can be shortened to:

	#!/usr/local/sbin/nastysan-config /real/config.dat

Notes:

- `nastysan-config` handles the files.  It is searched in the PATH as usual.
- Config files are identical on all clusters.
- So you can use any copy of the configuration file if you like.
- `nastysan-config` understoods SFTP, so you can do something like `remote.host:/real/config.dat`

It's all straight forward as expected.


## Configuration

NAStySAN has no API, UI or similar.  Everything is straight mapped into the F.U.S.E. directory tree.

Within the directory tree there are files, directories, softlinks and devices.

- Some directories can be created by you, like within `node/` to add more nodes
- Many files can be changed or created by you, these are configuration entries and are written to the configuration
- Devices are automatically created by NAStySAN based on the configuration
- Softlinks under `node/*/pool/` are used to add storage to NAStySAN.  Just soflink to a real device (or storage file).

Directory structure within the mountpoint:

- `stat/` various readonly information, including logging etc.
- `self/` read only configuration of your current node.
  - To make it configurable, copy it ot `node/` under a name you chose
- `node/` the defined nodes.  Configurable.
  - To create your node under this hood, do ``` cp -r self/. node/`hostname -f`/ ```
  - To add some node under this hood, do `cd node; mkdir NEWNODE; cd NEWNODE`, then edit as you like.
- `dev/` the devices which are configured on this node
  - Note that devices are exclusive to a node, so you cannot have them open on two nodes at the same time.
  - If more than one node has a device configured, only one node may "possess" the device, the others will never see it

More to come in future.


## `node/` confguration

T.B.D.


### `node/*/dev/` Device configuration

T.B.D.


### `node/*/pool/` Storage pool configuration

The storage pool is made of devices and or files which are added to the pool.

The catalog of known pool members is kept as softlinks under `node/NAME/pool/`.

- You can add and remove soflinks to enable/disable such pool members.
- Disabling a pool member means, that you can no more access the data on it.
- Radundancy usually ensures, that no data is missing.

A storage pool can be added even if it is not initialized yet.  This means it is cataloged, but is not yet enabled.

To enable such a member, use `nastysan-mkpool` command, giving the file or device.
This will only create a new pool if the first 16 MB and last 16 MB of the destination are empty (`NUL` or `00`-bytes).

- This is to not accidentally overwrite valuable data.
- This also means, that a minimum pool member size is 32 MB.

Note that this will show up as a `PV` later on, as `LVM` is used to manage pool members.


## Reliability

The data distribution of device contents is auto-managed on the storage pools with several reliability features:

- Redundancy: Data of devices is mirrored across several pool members.  The details are done in the device configuration.
- Integrity: Device data is made of timestamped checksums, so the correct contents can be located in the pool.
  - There also is a history trail (changelog), which allows to go back in time to find stable data
- Copy-On-Write: Device data is not immediately overwritten.
  - Changed pool data is written continuously into a new unused area (LVM extents).
  - The old data is not overwritten until there is no more free room.
  - Only then old stale data is invalidated and re-used for overwrite.
- Snapshots: You can snapshot device state any time.  This is a near-immediate operation.
- Fast reads: Data is automatically taken from the fastest known pool member
  - If a pool member unexpectedly takes longer, another pool member is tried very fast in parallel.
  - For example if your local harddrive is busy with a read error or relocating bad sectors automatically
    a second (networked pool member or other harddrive) is used.
- Fast writes: Writes are always fast, that is, they will nearly never wait nor fail if you need that.
  - Usually writes finish when the first pool member has accepted the data.
  - You can even use "unsafe" in which case the writes are always cached in memory.
  - Or you can require, that N pool members acknowledge the write.  It's up to you.
  - Write order still is strictly honoured, so you will never have any "write barrier" or reordering problem.
- Background operations
  - There are no foreground operations.
  - Things like RAID, deduplication (if ever supported) and so on always will be a background task
  - The only exceptions are things, which must be done offline (like a rebuild after some catastrophic event).
- Encryption:  In future, transparent encryption might be added as well
  - For now you can use LUKS for data security.
  - And `ssh` for transport security.  (NAStySAN supports `ssh` tunnel mode and direct `sftp`.)
  - However it would be nice to be able to not use LUKS but store data on other nodes encrypted.  This might be added in future.
  - Doing such a thing right is a PITA, as what happens if you loose your encryption keys.  This is why I leave this for future.


## Rationale

Things are failing.  Things are failing a lot.  Things are failing constantly.
And things are failing if you do not need them to fail.

With "The Cloud" you have some redundandcy.  But you do not always have or want The Cloud.

There are also nice things like Ceph.  Complex things like Ceph.  Uncomprehensible things like Ceph.

What you need is something, which just works.  Always.  The way you want it.  The way you need it.  The way you can do it.

This is why I create NAStySAN, which is a joke for "NAS type SAN".

- No, it's not a NAS, as it is no filesystem.
- No, it's not a SAN, as it is is not low level.


## License

This Works is placed under the terms of the Copyright Less License,
see file COPYRIGHT.CLL.  USE AT OWN RISK, ABSOLUTELY NO WARRANTY.

Read:  This is free as in free beer, free speech and free baby.

Read:  This code never can be copyrighted again.  Copyright is slavery and slavery must be abolished at all cost.  Or is here anybody who really wants to see a copyright on babies?


## FAQ

Python?

- Doing it in Python is more easy than in a compiled language.
- Also I like Python because it makes very readable.
- Python is completely up to the task for doing this.
- The Multithreading/GIL issue is no problem here, it even can be used to protect against unintended excess use of CPUs.
- Also we need to separate tasks, so we need process separation (aka `fork()`) anyway.
- Note that there might be a C/C++ rewrite in future when things stabilize.


Name?

- NAS stands for NAS
- ty stands for type
- SAN stands for SAN
- Please note that the domains nastysan.com/net/org/de are owned by me since 2001 already.


Contribute?  Contact?  Bugs?

- Please open issue or send PR at GitHub.  Eventually I listen.
- Waive copyright before sending pull requests.  Everything you send to me must be already covered by CLL.
- Try to keep the style.  Try to stay pythonic.


External libs?

- All must be FLOSS.
- If they are not part of Debian, they must become part of this as `git submodule`.


Python Module?

- I do not need it for me yet, but I will support it if someone does it.
- If you want to transform it according to the spec, please send pull request.


Debian packaging?

- If it has matured, I probably will add `gbp` to this to make it easy to dreate a Debian native package out of this.
- However I do not want to become a Debian maintainer:
  - I hate politics because it corrupts people because you cannot stay authentic while acting politically correct.  
    The fundamental problem with politically correctness is: Without authenticity there cannot be trust.  Period!
  - With the decision against Init Freedom, Debian started to become a high risk political system like Wikipedia.  
    I am not willing to become a part of such a beast, ever.


Politically Correctness and Code of Conduct?

- Discussions about Political Correctness or Code of Conduct are not tolerated.  Ever.  Period.  This is final.
- With the introduction of the Code of Conduct for the Linux Kernel, the Linux Kernel now is compromised.
  They sacrificed authenticity for Political Correctness.
- So I hereby inform you, that I never will obey Political Correctness nor accept any Code of Conduct, ever.
- If somebody ever asks me to for accepting a Code of Conduct or act Politically Correct in the internet, this is the process:
  - I answer this a single time to stop.
  - This then stops.  No objections, no discussion, no "but"s, nothing.  Just a "yes" or "accepted" or similar is allowed.
  - If anybody, related or not, will not stop, this will make me leave.
  - The leave is immediate and final.
  - Period.


CLL?

- You are free to drop this weird CLL, but then nothing informs you, that adding a Copyright breaks the law.

