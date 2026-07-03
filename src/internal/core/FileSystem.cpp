#include "FileSystem.hh"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace jstine {

const std::string& Path::str() const { return m_path; }

bool Path::isFile() const { return fs::is_regular_file(m_path); }

bool Path::isDirectory() const { return fs::is_directory(m_path); }

bool Path::exists() const { return fs::exists(m_path); }

Path Path::parent() const {
    auto parentPath = fs::path(m_path).parent_path();
    return Path{parentPath.string()};
}

Path Path::join(const Path& base, const Path& relative) {
    return Path{fs::path(base.str()) / fs::path(relative.str())};
}

bool Path::endsWith(const std::string& suffix) const {
    if (suffix.size() > m_path.size()) [[unlikely]] {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), m_path.rbegin());
}

void Path::append(const std::string& suffix) { m_path += suffix; }

File::File(const Path& path) : m_path(path) {}

const Path& File::path() const { return m_path; }

void File::append(const std::string& content) {
    std::ofstream file(m_path.str(), std::ios::app);
    if (not file.is_open()) {
        throw FileSystemError{
            ErrorCode::fileNotFound, "Failed to open file for appending: {}",
            m_path.str()
        };
    }
    file << content;
}

void File::write(const std::string& content) {
    std::ofstream file(m_path.str(), std::ios::trunc);
    if (not file.is_open()) {
        throw FileSystemError{
            ErrorCode::fileNotFound, "Failed to open file for writing: {}",
            m_path.str()
        };
    }
    file << content;
}

std::string File::read() const {
    std::ifstream file(m_path.str());
    if (not file.is_open()) {
        throw FileSystemError{
            ErrorCode::fileNotFound, "Failed to open file for reading: {}",
            m_path.str()
        };
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> File::readLines() const {
    std::ifstream file(m_path.str());
    if (not file.is_open()) {
        throw FileSystemError{
            ErrorCode::fileNotFound, "Failed to open file for reading: {}",
            m_path.str()
        };
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::vector<u32> File::readBinary() const {
    std::ifstream file(m_path.str(), std::ios::binary | std::ios::ate);
    if (not file.is_open()) {
        throw FileSystemError{
            ErrorCode::fileNotFound, "Failed to open file '{}' for reading",
            m_path.str()
        };
    }
    auto size = file.tellg();
    if (size % sizeof(u32) != 0) {
        throw FileSystemError{
            ErrorCode::invalidArgument,
            "File '{}' size is not a multiple of 4 bytes", m_path.str()
        };
    }
    file.seekg(0);
    std::vector<u32> buffer(static_cast<std::size_t>(size) / sizeof(u32));
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

void File::remove() {
    std::error_code ec;
    fs::remove(m_path.str(), ec);
    if (ec) {
        throw FileSystemError{
            ErrorCode::fileSystemError, "Failed to remove file '{}'",
            m_path.str()
        };
    }
}

Directory::Directory(const Path& path) : m_path(path) {}

const Path& Directory::path() const { return m_path; }

std::vector<Path> Directory::listFiles() const {
    std::vector<Path> files;
    for (const auto& entry : fs::directory_iterator(m_path.str())) {
        if (entry.is_regular_file()) {
            files.emplace_back(entry.path().string());
        }
    }
    return files;
}

std::vector<Path> Directory::listDirectories() const {
    std::vector<Path> directories;
    for (const auto& entry : fs::directory_iterator(m_path.str())) {
        if (entry.is_directory()) {
            directories.emplace_back(entry.path().string());
        }
    }
    return directories;
}

void Directory::create() {
    std::error_code ec;
    fs::create_directories(m_path.str(), ec);
    if (ec) {
        throw FileSystemError{
            ErrorCode::fileSystemError, "Failed to create directory '{}'",
            m_path.str()
        };
    }
}

void Directory::remove() {
    std::error_code ec;
    fs::remove_all(m_path.str(), ec);
    if (ec) {
        throw FileSystemError{
            ErrorCode::fileSystemError, "Failed to remove directory '{}'",
            m_path.str()
        };
    }
}

void Directory::createSubdirectory(const Path& name) {
    const auto subdirPath = Path::join(m_path, name);
    std::error_code ec;
    fs::create_directories(subdirPath.str(), ec);
    if (ec) {
        throw FileSystemError{
            ErrorCode::fileSystemError,
            "Failed to create subdirectory '{}' in '{}'", name.str(),
            m_path.str()
        };
    }
}

void Directory::touch(const Path& name) {
    const auto filePath = Path::join(m_path, name);
    std::ofstream file(filePath.str(), std::ios::app);
    if (not file.is_open()) {
        throw FileSystemError{
            ErrorCode::fileNotFound,
            "Failed to create or open file '{}' in '{}'", name.str(),
            m_path.str()
        };
    }
}

}  // namespace jstine
