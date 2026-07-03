#include "Error.hh"

namespace jstine {

Error::Error(Code code, const std::string& message)
    : m_code(code), m_message(message) {}

Error::Code Error::code() const { return m_code; }

const std::string& Error::message() const { return m_message; }

std::unexpected<Error> Error::unexpected(const Error& error) {
    return std::unexpected{error};
}

std::optional<Error> Error::empty() { return {}; }

}  // namespace jstine
