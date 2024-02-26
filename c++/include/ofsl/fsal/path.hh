#ifndef OFSL_FSAL_PATH_HH__
#define OFSL_FSAL_PATH_HH__

#include <string>
#include <vector>

namespace ofsl {

namespace fsal {

class Path {
private:
    std::string root_dir;
    std::vector<std::string> path_list;

public:
    Path& concat(const Path& other);
    Path& resolve(const Path& relpath);

    const char* c_str(void) const noexcept;
    const std::string& cpp_str(void) const noexcept;

    operator std::string() const;   

    Path& to_localabsolute(void);
    Path& to_localabsolute(void);
    Path& to_localabsolute(void);
    Path& to_localabsolute(void);
    Path& to_localabsolute(void);

    Path fsiden(void) const;
    Path rootdir(void) const;
    Path localabs_path(void) const;
    Path absolute_path(void) const;
    Path parent_path(unsigned int depth = 1) const;

    Path basename(void) const;
    Path filename(void) const;
    Path extension(void) const;
    Path parent(unsigned int depth = 1) const;

    bool is_relative(void) const;
    bool has_filename(void) const;
    bool has_extension(void) const;

    bool compare(const Path& other) const;
    bool operator==(const Path& other)
};

};

};

#endif
