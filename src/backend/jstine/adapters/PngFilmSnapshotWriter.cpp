#include "PngFilmSnapshotWriter.hh"

#include <algorithm>
#include <cmath>

#include "Stb.hh"
#include "jstine/core/Scope.hh"

namespace jstine {

namespace {

u8 linearToSrgb8(f32 linear) {
    if (not std::isfinite(linear)) {
        return 0;
    }

    linear = std::clamp(linear, 0.0f, 1.0f);

    const f32 encoded = linear <= 0.0031308f
                            ? 12.92f * linear
                            : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;

    return static_cast<u8>(std::lround(encoded * 255.0f));
}

}  // namespace

Result<void> PngFilmSnapshotWriter::write(
    const FilmSnapshot& snapshot, const Path& path
) {
    const auto& beauty = snapshot.beauty();
    const auto& extent = beauty.bounds().extent();
    auto pixels = beauty.pixels();

    i32 w = static_cast<i32>(extent.w);
    i32 h = static_cast<i32>(extent.h);
    constexpr i32 ch = 3;

    std::vector<u8> r;
    r.resize(w * h * ch);

    for (u32 i = 0; i < pixels.size(); ++i) {
        const auto& pixel = pixels[i];
        r[i * ch + 0] = linearToSrgb8(pixel.r);
        r[i * ch + 1] = linearToSrgb8(pixel.g);
        r[i * ch + 2] = linearToSrgb8(pixel.b);
    }

    if (stbi_write_png(path.str().c_str(), w, h, ch, r.data(), w * ch) == 0) {
        return Error::unexpected(
            ErrorCode::imageWriteError, "Failed to write PNG image (stb)"
        );
    }
    return {};
}

}  // namespace jstine
