# Introduction

dsktools is a set of tools for reading and writing DSK and EDSK images
to real floppy disks for use with the Amstrad/Schneider CPC range of
homecomputers. The main target platform for dsktools is GNU/Linux.

# Current State

dskwrite is able to write images in standard formats (SYSTEM,DATA) as
well as some special formats with unusual sector numbering and sector
sizes and deleted data (untested). There are no options for selecting
the drive, yet. Only the floppy disk in `/dev/fd0` is used.

dskread is much more complete than in older revisions. It understand
some copy protected formats and takes command line options for selecting
drive, side, number of sides and number of tracks. See dskread -h for a
list of known options.

# Compiling and Installing

Just type in `make`. Optionally copy the resulting binaries `dskread`
and `dskwrite` to some directory in your PATH, `/usr/local/bin` for
example. `make install` will copy them.

# Usage

Straight-forward, really.

``` Shell
./dskread <filename>
```


will read the disk in drive `/dev/fd0` and dump the contents into a DSK
image file. For non-standard formats there are a bunch of command line
options. See `dskread -h` for a list of available options.

``` Shell
./dskwrite [b] <filename>
```

will read the contents of a DSK image file and write it to a floppy disk
in drive `/dev/fd0`. If you put the `b` then write will occur to side B.

# Future

Future versions of `dskwrite` will handle different copy protection
schemes, faithfully reproducing the original disk from the image. I hope
to get this support as complete as it is in CPDWrite under DOS. I will
also implement some command line options to make dsktools more flexible
and user friendly.


# dsktools developers and contributors

main programming: Andreas Micklei <nurgle@gmx.de>

various fixes, deleted tracks writing, major parts of dskread: Kevin Thacker <KevinT@Core-Design.com>

multiple try on reading and writing: PulkoMandy

writing to side B: François Lacombe

inspiration, suggestions and example code: Ulrich Doewich <caprice32@cybercube.com>
