# Minix
Forked from [a9db0e](https://github.com/charlescao460/minix/commit/a9db0ea1844000e2d27fca2684bad61f2ab7a515)
 of [Stichting-MINIX-Research-Foundation/minix](https://github.com/Stichting-MINIX-Research-Foundation/minix).
 
 Added [run.sh](run.sh) and modified [x86_hdimage.sh](releasetools/x86_hdimage.sh) to make cross-build easier and faster.
 
## Dependencies
On Debian, run


```shell
apt-get install build-essential curl git zlibc zlib1g zlib1g-dev m4 qemu
```

To get all needed dependencies.

## Cross-Build
1. CD into repo root: `cd minix`
2. Run `sudo ./releasetools/x86_hdimage.sh FULL`
3. Run `run.sh` to boot Minix in QEMU.
4. Inside Minix, run `netconf` to config network if needed.

## Speed-Up
After first building, you can run `x86_hdimage.sh` without `FULL` argument:

```shell
sudo ./releasetools/x86_hdimage.sh
```

This will enable "Expert" mode in NetBSD's `build.sh`. 
And it will set `MKUPDATE`, `NOCLEANDIR` to `yes` to avoid repeated compile. Also `-T` option is set to use the toolchians that are previously built.


## Tuning
In [x86_hdimage.sh](releasetools/x86_hdimage.sh), the variable `$JOBS` is set to `16`, indicating the maximum number 
of parallel task is 16. You can change this value to further speed-up building process. Normally, it shoule be the number of your 
logical processors.


## Notes
Since the toolchian contains `gcc-4`, which are compiled based on outdated C98. Using newer version of gcc to build `gcc-4` would have
several issues. Related fixs are in [commit f66913](https://github.com/charlescao460/minix/commit/f66913f4a3d05c910d34343792be7199394487b8).
