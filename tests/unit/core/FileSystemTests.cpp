#include <gtest/gtest.h>

#include <filesystem>

#include "core/FileSystem.hh"

using namespace jstine;

namespace {

struct TempRoot {
    explicit TempRoot(const std::string& name)
        : path(std::filesystem::temp_directory_path() / name) {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }

    ~TempRoot() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }

    std::filesystem::path path;
};

}  // namespace

TEST(FileSystemTests, PathHelpersWork) {
    const Path base{"/tmp/jstine"};
    const Path child{"server"};
    const Path joined = Path::join(base, child);

    EXPECT_EQ(joined.str(), "/tmp/jstine/server");
    EXPECT_EQ(joined.parent().str(), "/tmp/jstine");
    EXPECT_TRUE(joined.endsWith("server"));
    EXPECT_FALSE(joined.endsWith("missing"));

    Path appended{"hello"};
    appended.append(".txt");
    EXPECT_EQ(appended.str(), "hello.txt");

    EXPECT_EQ(Path::fmt("{}/{}", "/tmp", "jstine").str(), "/tmp/jstine");
}

TEST(FileSystemTests, FileReadWriteAppendAndRemove) {
    TempRoot root{"jstine-file-system-file"};
    Directory dir{Path{root.path.string()}};
    ASSERT_FALSE(dir.create());

    const Path filePath = Path::join(dir.path(), Path{"note.txt"});
    File file{filePath};

    EXPECT_FALSE(file.write("hello"));
    ASSERT_TRUE(file.read());
    EXPECT_EQ(file.read().value(), "hello");

    EXPECT_FALSE(file.append(" world"));
    ASSERT_TRUE(file.read());
    EXPECT_EQ(file.read().value(), "hello world");
    EXPECT_TRUE(file.path().exists());
    EXPECT_TRUE(file.path().isFile());

    EXPECT_FALSE(file.remove());
    EXPECT_FALSE(file.path().exists());
}

TEST(FileSystemTests, DirectoryCanCreateAndListContents) {
    TempRoot root{"jstine-file-system-directory"};
    Directory dir{Path{root.path.string()}};
    ASSERT_FALSE(dir.create());

    EXPECT_FALSE(dir.createSubdirectory(Path{"nested"}));
    EXPECT_FALSE(dir.touch(Path{"item.txt"}));

    const auto files = dir.listFiles();
    const auto directories = dir.listDirectories();

    ASSERT_EQ(files.size(), 1u);
    ASSERT_EQ(directories.size(), 1u);
    EXPECT_TRUE(files.front().endsWith("item.txt"));
    EXPECT_TRUE(directories.front().endsWith("nested"));

    EXPECT_FALSE(dir.remove());
    EXPECT_FALSE(dir.path().exists());
}
