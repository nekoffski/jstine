#include "TomlConfigReader.hh"

#include <toml++/toml.hpp>

namespace jstine {

namespace {

class Reader : public NonCopyable, public NonMovable {
   public:
    explicit Reader(const toml::table& tbl) : m_tbl(tbl) {}

    template <typename T>
    Result<T> read(const Str& stanza, const Str& key) const {
        if (not m_tbl.contains(stanza)) {
            return Error::unexpected(
                ErrorCode::badConfig, "Config file must contain a [{}] table",
                stanza
            );
        }
        auto subTable = m_tbl.get(stanza)->as_table();

        if (not subTable) {
            return Error::unexpected(
                ErrorCode::badConfig, "Config file must contain a [{}] table",
                stanza
            );
        }

        if (not subTable->contains(key)) {
            return Error::unexpected(
                ErrorCode::badConfig,
                "Config file must contain a {} field in the [{}] table", key,
                stanza
            );
        }

        auto raw = subTable->get(key);

        if constexpr (std::is_same_v<T, std::vector<Str>>) {
            if (not raw->is_array()) {
                return Error::unexpected(
                    ErrorCode::badConfig,
                    "Config file field {}.{} must be an array", stanza, key
                );
            }
            std::vector<Str> result;
            for (const auto& item : *raw->as_array()) {
                if (not item.is_string()) {
                    return Error::unexpected(
                        ErrorCode::badConfig,
                        "Config file field {}.{} must be an array of strings",
                        stanza, key
                    );
                }
                result.push_back(item.value<Str>().value_or(""));
            }
            return result;
        } else {
            auto v = raw->value<T>();
            if (not v.has_value()) {
                return Error::unexpected(
                    ErrorCode::badConfig,
                    "Config file field {}.{} must be of the correct type",
                    stanza, key
                );
            }
            return *v;
        }
    }

   private:
    const toml::table& m_tbl;
};

#define READ_FIELD(lhs, stanza, key, type)            \
    do {                                              \
        auto result = reader.read<type>(stanza, key); \
        if (not result) {                             \
            return Error::unexpected(result.error()); \
        }                                             \
        lhs = result.value();                         \
    } while (0)

#define READ_FIELD_T(lhs, stanza, key, type, transform) \
    do {                                                \
        auto result = reader.read<type>(stanza, key);   \
        if (not result) {                               \
            return Error::unexpected(result.error());   \
        }                                               \
        lhs = transform(result.value());                \
    } while (0)

Result<void> readFields(Config& c, const Reader& reader) {
    READ_FIELD_T(c.log().level, "log", "level", Str, log::levelFromString);
    READ_FIELD(c.paths().output, "paths", "output", Str);
    READ_FIELD_T(
        c.renderer().backend, "renderer", "backend", Str,
        executionBackendFromString
    );
    READ_FIELD_T(
        c.renderer().integrator, "renderer", "integrator", Str,
        integratorAlgorithmFromString
    );

    return {};
}

}  // namespace

Result<void> TomlConfigReader::read(Config& config, const Path& path) const {
    try {
        auto tbl = toml::parse_file(path.str());
        Reader reader(tbl);
        return readFields(config, reader);
    } catch (const toml::parse_error& e) {
        return Error::unexpected(
            ErrorCode::badConfig, "Failed to parse config file '{}': {}",
            path.str(), e.what()
        );
    } catch (const std::exception& e) {
        return Error::unexpected(
            ErrorCode::badConfig, "Failed to read config file '{}': {}",
            path.str(), e.what()
        );
    }
    return {};
}

}  // namespace jstine
