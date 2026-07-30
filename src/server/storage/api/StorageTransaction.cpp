#include "StorageTransaction.hh"

namespace jstine {

StorageTransaction::StorageTransaction(Database& database)
    : m_database(database) {}

bool StorageTransaction::exists(CBytesView keyBytes) const {
    return m_database.exists(keyBytes);
}

void StorageTransaction::remove(CBytesView keyBytes) {
    m_database.remove(keyBytes);
}

Result<void> StorageTransaction::set(
    CBytesView keyBytes, CBytesView valueBytes
) {
    if (auto error = m_database.set(keyBytes, valueBytes); error) {
        return std::unexpected(*error);
    }
    return {};
}

Result<Bytes> StorageTransaction::get(CBytesView keyBytes) const {
    if (auto value = m_database.get(keyBytes); value) {
        return Bytes(value->begin(), value->end());
    } else {
        return std::unexpected(value.error());
    }
}

}  // namespace jstine
