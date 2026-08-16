#include "Point.hh"

namespace jstine {

Point2i Point2i::operator+(const Point2i& other) const {
    return Point2i{x + other.x, y + other.y};
}

}  // namespace jstine
