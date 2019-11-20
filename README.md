> This is just in the experimental phase, so PRE-PLANNING
>
> **State: Not ready to be used yet**

# NAStySAN

Distributed reliable redundant block device:

- Redundancy:  You can add and remove redundancy any time.
- Integrity: Timestamped checksums ensure data correctness and detect silent corruption
- Reliability: Things do not break as long as enough redundancy is available
- Fast Retry: If a Read is not satisfied in a short time, it is retried using the redundancy
- Fast Write: A Write is satisfied as soon as N devices acknowledged the data.  N=0 means memory caching
- Distributed: Storage can be locally or on the network

Future:

- Consistency: Not only write barriers are done, data is written in strict write order
- CopyOnWrite: Blocks are not overwritten as long as space permits
- Snapshots: Previous data can be kept by using a snapshot
- History: Semi-automatic point in time Snapshots allow to rollback data to a previous state
- Optimization: Optimizations always take place in the background (DeDup, high-level compression, etc.)
- Encryption: Data is encrypted before it leaves the local system


## Notes

This is **not stable** yet.  `/dev/nbd` also is a very deep part of the kernel.
Hence, even that this is userland, you can make your machine unusable.

> **YOU HAVE BEEN WARNED!**

Note that the kernel does not crash immediately, but slowly degrades usabilty.

But you can expect more and more things to fail, which even makes the OS no more
shutdown.  For example unkillable processes, permanently locked in memory,
waiting for IO of a `/dev/nbdX` which can no more happen.  This is especially
true for some most basic commands like `/bin/sync`.  They then make scripts
block and pile up until everything is too late.  And more advanced commands
like `lvm` become immediately unusable, as they try to access `/dev/nbdX`
and become blocked, too.

`echo b >/proc/sysrq-trigger` is your friend then.

The only lucky part is that those locked processes do not eat CPU.
However they eat more and more memory and process IDs until everything
is eaten up by chance.  Then your machine stops responding.

> **YOU HAVE BEEN WARNED!**

Note that `nbd-client -d` does not help either at my side.  A there are
`nbd` devices hung in the kernel, the `nbd` kernel module becomes unremovable, too.

Effectively the only thing you can do then, is to hard-reset your machine
to revive it.

YMMV, I am experimenting with Ubuntu 18.04 and Kernel `4.15.0-58-lowlatency`.

> Recommendation:  Do experiments in a scratch VM.  And be very careful when
> using `nbd` in production.
>
> This does not render production instable or unusable per-se.  But it will
> create a lot of headache until you can reboot the machine.

Note that this probably is not the entire fault of this userland here.
The kernel should make sure that things cannot go sideways this way.  Ever.

However I am working on this, because I do not think things are production
ready if they can make your machine fail without no chance to repair,
besides a hard reboot.

Perhaps this is just a lack of documentation an tooling.  So I did not
find a tool or way yet, to "power detach" a stuck `/dev/nbd`.

> There is a trick.  Use a timeout.  If there is a timeout, `/dev/nbd` will
> timeout stuck calls, making them fail, such that you, slowly, can
> disconnect a stuck /dev/nbd.
>
> Without I want to run it without any timeout, so things become very ugly.


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

Name?

- NAS stands for NAS
- ty stands for type
- SAN stands for SAN
- Please note that the domains nastysan.com/net/org/de are owned by me since 2001 already.


Contribute?  Contact?  Bugs?

- Please open an issue or send a PR at GitHub.  Eventually I listen.
- Waive copyright before sending pull requests.  Everything you send to me must be already covered by CLL.


External libs?

- All must be FLOSS.
- If they are not part of Debian, they must become part of this as `git submodule`.


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

