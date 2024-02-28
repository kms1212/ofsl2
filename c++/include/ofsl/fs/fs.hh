#ifndef OFSL_FS_FS_HH__
#define OFSL_FS_FS_HH__

#include <cstdlib>

#include <ofsl/fs/fs.h>

#include <string>

#if __cplusplus < 201703L  // std::iterator is deprecated in C++17
#include <iterator>
#endif

namespace ofsl {

typedef OFSL_MountInfo MountInfo;
typedef OFSL_FileAttributeType FileAttributeType;

class FileSystem {

};

class directory_iterator;

class Directory {
public:

private:
    OFSL_Directory* dir;

public:
    Directory(Directory* parent, const std::string& name);
    ~Directory();

    OFSL_Directory* get_c_obj(void);
    const OFSL_Directory* get_c_obj(void) const;

    static int Create(Directory* parent, const std::string& name);
    static int Remove(Directory* parent, const std::string& name);
};

class directory_iterator
#if __cplusplus < 201703L  // Before C++17, inherit std::iterator
: public std::iterator<std::input_iterator_tag, FileAttribute>
#endif
{
private:
    OFSL_DirectoryIterator* it;
    FileAttribute fattr;
    bool is_end;

public:
    directory_iterator(Directory& dir) :
        it(ofsl_dir_iter_start(dir.get_c_obj()))
    {
        this->is_end = false;
    }
    
    directory_iterator& operator++()
    {
        this->is_end = ofsl_dir_iter_next(this->it) != 0;
        return *this;
    }

    bool operator==(const directory_iterator& other) const
    {
        return this->is_end == other.is_end;
    }

    bool operator!=(const directory_iterator& other) const
    {
        return !(*this == other);
    }

    FileAttribute& operator*()
    {
        ofsl_dir_iter_get_attr(this->it, &this->fattr);
        return this->fattr;
    }

    static directory_iterator& end(void);

#if __cplusplus >= 201703L  // After C++17, implement iterator traits
    using value_type = FileInfo;
    using difference_type = long;
    using pointer = const FileInfo*;
    using reference = const FileInfo&;
    using iterator_category = std::forward_iterator_tag;
#endif
};



class File {

};

};

#endif
