# read_track

Simple program to access content of floppy sectors or tracks for AmigaOS 3.

While reversing old games to learn game dev techniques,
I stumbled upon the need to extract specific sectors
and tracks.
As an exercice, I coded this small utility.


## Usage
```
USAGE: read_track {-t <track> | -s <sector>} [-c <count>] [-o <outfile>] [-d <unit>] [-v]
        -c <count>, count of tracks or sectors to read, default to 1
        -d <unit>, floppy disk unit, defaults to 0 (df0:)
        -o <outfile>, file to write the track(s) to, as binary,
                      default is to output as hexdump
        -s <sector>, number of first sector to read, 0..79
        -t <track>, number of first track to read, 0..79
        -v, displays more info, like drive geometry
```

## Compiling

I compile it with VBCC 0.9 with posix library and NDK3.2R4.

Included is dbg/dbg.h, some useful debug macros.
