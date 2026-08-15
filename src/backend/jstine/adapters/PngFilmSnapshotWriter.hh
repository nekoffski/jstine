#pragma once

#include "jstine/imaging/Film.hh"

namespace jstine {

class PngFilmSnapshotWriter final : public FilmSnapshot::Writer {
   public:
    Opt<Error> write(const FilmSnapshot& snapshot, const Path& path) override;

   private:
};

}  // namespace jstine