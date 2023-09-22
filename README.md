# Cache Management

**Some concepts and design ideas I have for cache management.**

Ever wondered where all your free disk space went?
Perhaps you are using package managers.

> We live in a time where we need to overengineer cache management if we don't want our `~` folders
> to be cluttered with tons of garbage or disk space to be claimed infinitely like it's free real estate.

## Idea 1: compressed filesystem + symlinks or bind mounts

Use filesystems with transparent compression, like btrfs,
in combination with a disk image of limited size.

The idea is to limit cache build up, so it doesn't claim
your entire free disk space. Your storage isn't free real estate after all.

**Note:** `1000` is meant to be the UID to take multi user systems into consideration.

**Note 2:** All paths are used for demonstration purposes, adjust them to your personal preferences.

**1. Create a disk image which will contain all the caches.**

In this example the disk image will be 32GB in size to limit the total amount of available disk space for caches.

`dd if=/dev/zero of=/var/lib/diskimages/cache-1000.img bs=1G count=32 status=progress`

**2. Create a btrfs filesystem on the disk image.**

Default settings work just fine. `mkfs.btrfs` is smart enough
to detect that the filesystem isn't created on a physical
disk, but rather on a file on top of an existing filesystem.

`mkfs.btrfs -L cache-1000 -f /var/lib/diskimages/cache-1000.img`

**3. Mount the btrfs filesystem with compression enabled.**

Add this to your `/etc/fstab`:

```fstab
/var/lib/diskimages/cache-1000.img /caches/1000 btrfs loop,compress=lzo,nodiscard,noatime,nodev,nosuid 0 0
```

Upon the first mount, run `chown -R 1000:1000 /caches/1000` to give the relevant user r/w access to the cache directory.

**Note:** `noexec` would be a nice mount option, but some package
managers assume that binaries inside its cache directory
can be executed and will fail in unexpected ways if they
can't do so. Whenever you want to add the `noexec` option
is up to you and your specific use case.

> In my personal opinion, cache directories should not contain native binaries (which are meant to be executed inside this directory) and must work with `exec` capabilities disabled.
>
> `noexec` will definitely fail for Gradle and npm, depending on what libraries you are pulling in, from my personal experience.

**4. Create symlinks for various cache directories.**

This step could be automatated nicely using a script.

Basically you just remove (or move) the existing cache directory and create a symlink into the compressed filesystem.

This is the output of the script [`./bin/list-cache-dirs`](./bin/list-cache-dirs) on my system:

```plain
Persistent caches in ~
    /home/magiruuvelvet/.cargo -> /caches/1000/cargo
    /home/magiruuvelvet/.dartServer -> /caches/1000/dartServer
    /home/magiruuvelvet/.dub -> /caches/1000/dub
    /home/magiruuvelvet/.go -> /caches/1000/go
    /home/magiruuvelvet/.gradle -> /caches/1000/gradle
    /home/magiruuvelvet/.m2 -> /caches/1000/m2
    /home/magiruuvelvet/.node-gyp -> /caches/1000/node-gyp
    /home/magiruuvelvet/.npm -> /caches/1000/npm
    /home/magiruuvelvet/.pub-cache -> /caches/1000/pub-cache
Persistent caches in ~/.cache
    /home/magiruuvelvet/.cache/composer -> /caches/1000/composer
    /home/magiruuvelvet/.cache/go-build -> /caches/1000/go-build

Available caches on the compressed disk image
    /caches/1000 (7.2G)
    /caches/1000/npm (274M)
    /caches/1000/pub-cache (284M)
    /caches/1000/m2 (64M)
    /caches/1000/gradle (4.1G)
    /caches/1000/dartServer (289M)
    /caches/1000/go-build (0)
    /caches/1000/cmake (0)
    /caches/1000/node_modules (2.3G)
    /caches/1000/composer (0)
    /caches/1000/cargo (0)
    /caches/1000/go (0)
    /caches/1000/dub (0)
    /caches/1000/README.md (4.0K)
    /caches/1000/node-gyp (16M)

BTRFS stats
Data, single: total=5.38GB, used=4.35GB
System, DUP: total=8.39MB, used=16.38kB
Metadata, DUP: total=536.87MB, used=392.74MB
GlobalReserve, single: total=13.39MB, used=0.00B
WARNING: cannot read detailed chunk info, per-device usage will not be shown, run as root
Overall:
    Device size:                           34.36GB
    Device allocated:                       6.47GB
    Device unallocated:                    27.89GB
    Device missing:                          0.00B
    Device slack:                          18.45EB
    Used:                                   5.14GB
    Free (estimated):                      28.92GB      (min: 14.97GB)
    Free (statfs, df):                     28.92GB
    Data ratio:                               1.00
    Metadata ratio:                           2.00
    Global reserve:                        13.39MB      (used: 0.00B)
    Multiple profiles:                          no

Data,single: Size:5.38GB, Used:4.35GB (80.90%)

Metadata,DUP: Size:536.87MB, Used:392.74MB (73.15%)

System,DUP: Size:8.39MB, Used:16.38kB (0.20%)
```

While some package managers offer environment variables, or
other configuration options to change their cache directory.
Not all package managers do so. The only options in this case
are either symbolic links or bind mounts.

If a package manager refuses to work with symbolic links, the
only option is to use bind mounts. *(or nicely ask the developers if they could improve cache handling)*

### Pros/Cons of this method

 `+` caches are limited to a defined amount of storage and can't claim your entire disk space

 `+` if you wan't to get rid of all caches, you can simply recreate the filesystem on the disk image, or run remove operations on it without worrying to accidentally delete important files

 `-` the disk image itself has a static size and permanently requires storage of its size

