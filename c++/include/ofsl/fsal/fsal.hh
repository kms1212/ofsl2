#ifndef OFSL_FSAL_FSAL_HH__
#define OFSL_FSAL_FSAL_HH__

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#include <string>

#include <ofsl/drive/drive.hh>
#include <ofsl/fsal/path.hh>

namespace ofsl {

namespace fsal {

class State;

class Directory {

};

class File {
public:
    File(State* state, const Path& filename, const char* mode);
    File(const Path& filename, const char* mode);
    ~File();
    
    int close(void);
    int flush(void);

    size_t read(void* buffer, size_t size, size_t count);
    size_t write(const void* buffer, size_t size, size_t count);

    int getc(void);
    char* gets(char* str, int count);
    int putc(int ch);
    int puts(const char* str);
    int ungetc(int ch);

    __attribute__((format(scanf, 2, 3)))
    int scanf(const char* format, ...);

    __attribute__((format(printf, 2, 3)))
    int printf(const char* format, ...);

    ssize_t tell(void);
    int getpos(fpos_t* pos);
    int seek(ssize_t offset, int origin);
    int setpos(const fpos_t* pos);
    void rewind(void);
};

class State {
public:
    State(void);
    
    static State* GetGlobalState(void);
};

std::string register_drive(OFSL_Drive* drv);
void unregister_drive(const std::string& id);

int mount_fs(const std::string& id);
int unmount_fs(const std::string& id);

};

};

#endif
