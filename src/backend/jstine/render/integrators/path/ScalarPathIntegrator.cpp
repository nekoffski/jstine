#include "ScalarPathIntegrator.hh"

namespace jstine {

ScalarPathIntegrator::ScalarPathIntegrator(const Config::Renderer& config) {}

Result<void> ScalarPathIntegrator::render(RenderContext& context) {
    const auto& bounds = context.film.pixelBounds();
    const auto min = bounds.min();
    const auto max = bounds.max();
    const auto extent = bounds.extent();

    const auto inverseWidth = 1.0f / static_cast<f32>(extent.w);
    const auto inverseHeight = 1.0f / static_cast<f32>(extent.h);

    for (i32 y = min.y; y < max.y; ++y) {
        for (i32 x = min.x; x < max.x; ++x) {
            const auto u = (static_cast<f32>(x - min.x) + 0.5f) * inverseWidth;
            const auto v = (static_cast<f32>(y - min.y) + 0.5f) * inverseHeight;

            context.film.addRgbSample(
                Point2i{x, y}, LinearRgb{u, v, 0.25f}, 1.0f
            );
        }
    }

    return {};
}

}  // namespace jstine
