#include "Film.hh"

namespace jstine {

const Plane<BeautyTag>& jstine::FilmSnapshot::beauty() const {
    return m_beauty;
}

}  // namespace jstine
