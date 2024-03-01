# OpenFSL 2
![BSD 3-Clause License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)
[![build](https://github.com/kms1212/ofsl2/actions/workflows/build.yml/badge.svg)](https://github.com/kms1212/ofsl2/actions/workflows/build.yml)
[![codecov](https://codecov.io/gh/kms1212/ofsl2/graph/badge.svg?token=OQQ5TAQUKP)](https://codecov.io/gh/kms1212/ofsl2)

This is a library for manipulating file system structures inside the physical drive or disk image implemented on C99 and C11 partially.
There is an C++ wrapper supports C++11 or higher and it can be used with the C header.
Currently it supports only little-endian systems such as i386, AMD64, arm64.

## Motivation

I wanted to rewrite the [OpenFSL](https://github.com/kms1212/OpenFSL) which began as a C++ library, because its code is horribly messy and complicated.
And there's one more reason that I had to implement several filesystems while developing firmware for the [EHBC](https://github.com/ExtensibleHomeBrewComputer) project; which targets to build a homebrew computer.
Since there's no any library implementing multiple filesystems and adding an abstration layer that enables to control files on multiple filesystems and disks, I started this project.

## How To Build & Test

This library requires `CMake >= 3.13`, `gcc` or gcc-compatible compilers like `clang`, and `zlib` to build.
`zlib` dependency will be optional in future.

The following shell script will build the library:
```shell
git clone https://github.com/kms1212/ofsl2.git
cd ofsl2
cmake -S. -Bbuild -DBUILD_TESTING=TRUE
cmake --build build
```

And the following script will test its features working properly:
```shell
ctest --test-dir build/tests --output-on-failure
```

## Configure Options

Option Name                       | Type    | Description
----------------------------------|---------|-------------
`LIBRARY_TYPE`                    | String  | Specify the library binary type.<br/>Possible values: `SHARED` or `STATIC`
`FORCE_STD_C99`                   | Bool    | Force using C99 standard. Using C99 disables the usage of `struct timespec` in `time.h`, therefore the clock precision is limited to unit of a second.
`USE_ZLIB`                        | Bool    | Use zlib if available.
`BUILD_TESTING`                   | Bool    | Build test binaries
`BUILD_CXX_WRAPPER`               | Bool    | Build C++ wrapper library
`BUILD_FILESYSTEM_<fs>`           | Bool    | Build the given filesystem support.<br/>[Placeholder Values](#supporting-filesystems)
`BUILD_FILESYSTEM_<fs>_EXTENSION` | List    | Build the specified extensions support of the filesystem.<br/>Extension names may vary for each filesystem.
`BUILD_PTABLE_<partition table>`  | Bool    | Build the given partition table support.<br/>[Placeholder Values](#supporting-partition-tables)
`BUILD_FSAL`                      | Bool    | Build the Filesystem Abstraction Layer
`CMAKE_BUILD_TYPE`                | String  | Specify the build type.<br/>Possible values: `Debug` or `Release`
`CMAKE_INSTALL_PREFIX`            | Path    | Specify the path where the library to install.
`GENERATE_COVERAGE`               | Bool    | Add flags to the compiler to make the library to generate coverage database for test coverage analyzation.

## Supporting Filesystems & Extensions

Filesystem  | Identifier | Constraints             | Extensions
------------|------------|-------------------------|-------------------
FAT12/16/32 | `FAT`      | Read only, Experimental | `LFN`
ISO9660     | `ISO9660`  | Read only, Experimental | `JOILET` `ROCKRIDGE`
ext2/3/4    | `EXT`      | Pending                 |
exFAT       | `EXFAT`    | Pending                 |
NTFS        | `NTFS`     | Pending                 |
minix       | `MINIXFS`  | Pending                 |
UDF         | `UDF`      | Pending                 |

## Supporting Partition Tables

Partition Table | Constraints
----------------|-----------------------------
GPT             | Read only, Experimental
MBR             | Pending

## The Filesystem Abstration Layer (FSAL)

The FSAL is a interface that enables the programmer to manipulate OpenFSL similar with the standard C library by the functions like `fgets`, `fprintf`, etc. 

### Path String Format

The FSAL has its own path string format looks similar to that of MS-DOS and Windows.

#### Volume Identifier
```
[Drive Type]N(pM)
```
**examples**: `fd0` `cdrom0` `hd0p2`

The number N and M is given in FCFS basis starting from 0 when the drive or the volume are mounted and registered.
If there's no partition table detected from the disk, the whole disk will be recognized as a partition.

#### Absolute Path
```
[Volume Identifier]:/<directory>/.../<directory>(/filename>)
```
**examples**: `fd0:/foo/bar.baz` `fd1:/` `cdrom0:/helloworld.txt` `hd0fs1:/foo/./../foo/bar.baz`

An absolute path string starts with file system specifier and is separated by single colon and single slash with other path string components.

#### Local Absolute Path
```
/<directory>/.../<directory>(/<filename)
```
**examples**: `/` `/foo/bar.baz`

A local absolute path starts with a single slash specifies the path hierarchy beginning from the root directory of the volume of the current path.

#### Relative Path
```
(<directory>/)<directory>/.../<directory>(/filename>)
```
**examples**: `./foo/` `bar/baz` `../../src/main.c`

### Usage with FSAL

These are some examples in C and C++.
Two examples works identically: open and register a raw disk image, do some operations, open, read, and close a file, and clean up.
```c
#include <ofsl/fsal/fsal.h>
#include <ofsl/fsal/io.h>
#include <ofsl/drive/rawimage.h>
#include <ofsl/ptbl/gpt.h>
#include <ofsl/fs/fat.h>

int main(int argc, char** argv)
{
    OFSL_Drive* image = ofsl_create_rawimage_drive("disk.img", 0, 512);
    fsal_register_drive(image, "hd");
    fsal_mount("hd0p0");

    fsal_gchdir("hd0p0:/");

    fsal_gmkdir("directory");
    fsal_gchdir("directory");
    fsal_gpwd();  /* "hd0p0:/directory" */

    fsal_gchdir("/");
    fsal_gpwd();  /* "hd0p0:/" */

    FSAL_State local_state;
    fsal_init_state(&local_state);
    fsal_chdir("hd0p1:/directory");
    fsal_pwd(&local_state);  /* "hd0p1:/directory" */

    FSAL_File* file = fsal_gfopen("file.txt", "r");
    char line_buf[128];
    fsal_fgets(line_buf, sizeof(line_buf), file);
    fsal_fclose(file);

    fsal_unmount_fs("hd0p0");
    fsal_unregister_drive("drv0");
    ofsl_drive_delete(image);

    return 0;
}
```

```c++
#include <ofsl/fsal/fsal.hh>
#include <ofsl/drive/rawimage.hh>
#include <ofsl/fs/fat.hh>

int main(int argc, char** argv)
{
    ofsl::RawImage image("disk.img", 0, 512);
    ofsl::fsal::register_drive(&image, "hd");

    ofsl::fsal::mount_fs("hd0p0");

    ofsl::fsal::chdir("hd0p0:/");

    ofsl::fsal::mkdir("directory");
    ofsl::fsal::chdir("directory");
    ofsl::fsal::pwd();  /* "hd0p0:/directory" */

    ofsl::fsal::chdir("/");
    ofsl::fsal::pwd();  /* "hd0p0:/" */

    ofal::fsal::State local_state;
    local_state.chdir("hd0p1:/directory");
    local_state.pwd(&local_state);  /* "hd0p1:/directory" */

    ofsl::fsal::File* file("file.txt", "r");
    char line_buf[128];
    file.fgets(line_buf, sizeof(line_buf));
    file.fclose();

    ofsl::fsal::unmount_fs("hd0fs0");
    ofsl::fsal::unregister_drive(image_drvid);

    return 0;
}
```
