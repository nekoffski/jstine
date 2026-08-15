#include "PngFilmSnapshotWriter.hh"

#include "Stb.hh"
#include "jstine/core/Scope.hh"

namespace jstine {

Opt<Error> PngFilmSnapshotWriter::write(
    const FilmSnapshot& snapshot, const Path& path
) {
    const auto& beauty = snapshot.beauty();
    const auto& extent = beauty.bounds().extent();
    auto pixels = beauty.pixels();

    i32 w = static_cast<i32>(extent.x);
    i32 h = static_cast<i32>(extent.y);
    i32 ch = static_cast<i32>(beauty.channels());

    if (stbi_write_png(path.str().c_str(), w, h, ch, pixels.data(), w * ch)) {
        return Error{
            ErrorCode::imageWriteError, "Failed to write PNG image (stb)"
        };
    }
    return {};
}

}  // namespace jstine