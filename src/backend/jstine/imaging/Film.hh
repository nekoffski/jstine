#pragma once

#include "Color.hh"
#include "Plane.hh"
#include "RasterDomain.hh"
#include "jstine/core/Concepts.hh"
#include "jstine/core/Error.hh"
#include "jstine/core/FileSystem.hh"

namespace jstine {

struct FilmSpec {};

class Film;

class FilmSnapshot final {
    friend class Film;

   public:
    struct Writer : public NonCopyable, public NonMovable {
        virtual ~Writer() = default;
        virtual Opt<Error> write(
            const FilmSnapshot& snapshot, const Path& path
        ) = 0;
    };

    const Plane<BeautyTag>& beauty() const;

   private:
    Plane<BeautyTag> m_beauty;
};

class Film final : public NonCopyable {
   public:
    Result<FilmSnapshot> snapshot() {}
};

}  // namespace jstine
