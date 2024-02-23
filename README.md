# OpenFSL 2

This is a library for manipulating file system structures inside the physical drive or disk image implemented on C99.
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

## The FileSystem Abstration Layer (FSAL)

The FSAL is a interface that enables the programmer to manipulate OpenFSL similar with the standard C library by the functions like `fgets`, `fprintf`, etc. 

### Path String Format

The FSAL has its own path string format looks similar to that of MS-DOS and Windows.

#### File System Specifier
```
drvNfsM
```
**examples**: `drv0fs0` `drv1fs0` `drv0fs1`

The number N and M is given in FCFS basis when the drive or the filesystem are registered or mounted.

#### Absolute Path
```
[File System Specifier]:/<directory>/.../<directory>(/filename>)
```
**examples**: `drv0fs0:/foo/bar.baz` `drv0fs1:/helloworld.txt` `drv0fs1:/foo/./../foo/bar.baz`

An absolute path string starts with file system specifier and colon-separated with other path string components.
A trailing slash is allowed when 

#### Local Absolute Path
```
/<directory>/.../<directory>(/<filename)
```
**examples**: `/foo/bar.baz`

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
    OFSL_Drive* image = ofsl_create_rawimage_drive("disk.img", 0, 512, 1024);
    fsal_register_drive(image);
    fsal_mount_fs("drv0fs0");

    fsal_gchdir("drv0fs0:/");

    fsal_gmkdir("directory");
    fsal_gchdir("directory");
    fsal_gpwd();  // "drv0fs0:/directory"

    fsal_gchdir("/");
    fsal_gpwd();  // "drv0fs0:/"

    FSAL_State local_state;
    fsal_init_state(&local_state);
    fsal_chdir("drv0fs1:/directory");
    fsal_pwd(&local_state);  // "drv0fs0:/directory"

    FSAL_File* file = fsal_gfopen("file.txt", "r");
    char line_buf[128];
    fsal_fgets(line_buf, sizeof(line_buf), file);
    fsal_fclose(file);

    fsal_unmount_fs("drv0fs0");
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
    ofsl::RawImage image("disk.img", 0, 512, 1024);

    ofsl::fsal::register_drive(&image);

    ofsl::Drive* image = ofsl_create_rawimage_drive("disk.img", 0, 512, 1024);
    std::string image_drvid = fsal_register_drive(image);
    ofsl::fsal::mount_fs(image_drvid + "fs0");

    ofsl::fsal::chdir(image_drvid + "fs0:/");

    ofsl::fsal::mkdir("directory");
    ofsl::fsal::chdir("directory");
    ofsl::fsal::pwd();  // "drv0fs0:/directory"

    ofsl::fsal::chdir("/");
    ofsl::fsal::pwd();  // "drv0fs0:/"

    ofal::fsal::State local_state;
    local_state.chdir("drv0fs1:/directory");
    local_state.pwd(&local_state);  // "drv0fs0:/directory"

    ofsl::fsal::File* file("file.txt", "r");
    char line_buf[128];
    file.fgets(line_buf, sizeof(line_buf));
    file.fclose();

    ofsl::fsal::unmount_fs(image_drvid + "fs0");
    ofsl::fsal::unregister_drive(image_drvid);

    return 0;
}
```
