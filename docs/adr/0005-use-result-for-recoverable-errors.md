# Use Result for recoverable errors

Recoverable failures at configuration, import, I/O, implementation-selection, device, and Render Session seams will use a thin project `Result<T>` abstraction based on C++23 `std::expected<T, Error>`. Internal invariant violations use assertions or contract-style checks, hot kernels use compact domain results, exceptions are not normal control flow or allowed to cross worker, C, or device seams, and exceptions from dependencies are translated by their adapters rather than requiring exceptions to be disabled project-wide.
