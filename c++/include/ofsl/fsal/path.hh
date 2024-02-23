#ifndef OFSL_FSAL_PATH_HH__
#define OFSL_FSAL_PATH_HH__

#include <string>

namespace ofsl {

namespace fsal {

struct Path {
    std::string str;

    Path& concat(const Path& other);
    Path& resolve(const Path& relpath);

    Path get_basename(void);
    Path get_extension(void);
    Path get_rootdir(void);
    Path get_fsid(void);
    Path get_parent(void);
    
    bool is_valid(void);
    bool is_relative(void);
    bool has_filename(void);
    bool has_extension(void);
};

};

};

#endif
