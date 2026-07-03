#pragma once

#include <string>
#include <vector>

#include "Core.hh"
#include "Error.hh"

namespace jstine {

DEFINE_ERROR(FileSystemError);

class Path {
   public:
    template <typename... Args>
        requires std::constructible_from<std::string, Args...>
    Path(Args&&... args) : m_path(std::forward<Args>(args)...) {}

    template <typename... Args>
    static Path fmt(const std::string& fmt, Args&&... args) {
        return Path(
            fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...)
        );
    }

    const std::string& str() const;

    bool isFile() const;
    bool isDirectory() const;
    bool exists() const;

    Path parent() const;

    static Path join(const Path& base, const Path& relative);

    bool endsWith(const std::string& suffix) const;
    void append(const std::string& suffix);

   private:
    std::string m_path;
};

class File {
   public:
    explicit File(const Path& path);

    const Path& path() const;

    void append(const std::string& content);
    void write(const std::string& content);
    std::string read() const;
    std::vector<std::string> readLines() const;
    std::vector<u32> readBinary() const;

    void remove();

   private:
    Path m_path;
};

class Directory {
   public:
    explicit Directory(const Path& path);

    const Path& path() const;

    std::vector<Path> listFiles() const;
    std::vector<Path> listDirectories() const;

    void create();
    void createSubdirectory(const Path& name);
    void touch(const Path& name);
    void remove();

   private:
    Path m_path;
};

}  // namespace jstine
