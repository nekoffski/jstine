#pragma once

#include "jstine/core/FileSystem.hh"
#include "jstine/imaging/Film.hh"

namespace jstine {

class PngFilmSnapshotWriter final : public NonCopyable, public NonMovable {
   public:
    Result<void> write(const FilmSnapshot& snapshot, const Path& path);
};

}  // namespace jstine