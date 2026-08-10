# Modern C++ Engineering Standards: Enforceable Options

_Research note, 8 August 2026. Sources are limited to the C++ Core Guidelines, WG21 papers, and official compiler, sanitizer, CMake, Conan, and MSVC documentation. This is a menu of enforceable practices and explicit trade-offs for a C++23 rendering workbench; it is not an adopted project policy._

## Executive summary

A useful engineering standard has two layers:

1. **Mechanically enforced safety and build rules:** ISO C++23 mode, explicit ownership, RAII, no non-constant global state, target-scoped build requirements, warning-clean project code, formatting, selected static checks, sanitizer jobs, and a multi-compiler/multi-architecture build matrix.
2. **Recorded design decisions:** exceptions versus `std::expected`, how broad a context object may be, which semantic quantities deserve distinct types, acceptable warning noise, minimum tool versions, and how much reproducibility the dependency and benchmark profiles require.

The distinction matters. The C++ Core Guidelines explicitly mix precisely checkable rules with heuristics and general principles, and recommend project-specific subsets rather than blind application of every rule ([Guidelines scope and enforcement](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#inforce-enforcement)). Formatting and `-Werror` are easy gates; “good abstraction” is not.

## Enforceability map

| Practice | Primary enforcement | Strength |
|---|---|---|
| C++23, no compiler language extensions | CMake target properties and all three compilers | Deterministic |
| No direct owning raw pointers or naked `new`/`delete` | API types, clang-tidy, review | Strong but not complete |
| No mutable namespace/global state | clang-tidy plus narrow documented exceptions | Strong heuristic |
| RAII for every acquired resource | types, sanitizer tests, review | Strong design rule |
| Warning-clean project targets | compiler and CI | Deterministic per pinned compiler |
| Formatting | pinned clang-format and CI | Deterministic |
| Data-race freedom | ownership design, TSan, stress tests, review | Dynamic/heuristic |
| Strong semantic types | type system, deleted/explicit operations | Deterministic once types exist |
| Dependency injection and bounded contexts | interfaces, architecture tests/review | Structural but partly subjective |
| Numerically portable behavior | scalar reference, tests on compiler/ISA matrix | Empirical |
| Readable names and “right-sized” modules | review | Subjective |

## Ownership, lifetime, and interface vocabulary

The default should be a value. Introduce indirection only when identity, optionality, polymorphism, a borrowed view, or a distinct lifetime requires it. The Core Guidelines recommend RAII, treat a raw pointer as non-owning, prefer `unique_ptr` over `shared_ptr`, and reserve smart-pointer parameters for expressing lifetime semantics ([R.1 RAII](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-raii), [R.3 raw pointers](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-ptr), [R.20–R.21 smart ownership](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-owner), [R.30 smart-pointer parameters](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-smartptrparam)).

| Interface form | Meaning to standardize | Important limitation |
|---|---|---|
| `T` | Independent value; copy or move according to the type | Avoid expensive accidental copies; measure before replacing clear value semantics |
| `const T&` | Required borrowed input for the duration of the call | Must not be retained unless a longer lifetime is explicitly guaranteed |
| `T&` | Required borrowed input/output | Mutation is part of the interface and should be narrow |
| `const T*` / `T*` | Nullable, non-owning borrow | A pointer alone does not carry an extent or lifetime |
| `std::span<const T>` / `std::span<T>` | Borrowed contiguous sequence, passed by value | The owner must outlive the span; `const span<T>` does not make elements const |
| `std::unique_ptr<T>` | Exclusive ownership; by value transfers it | Do not use merely to avoid declaring a stable owner |
| `std::shared_ptr<T>` | Shared lifetime ownership | Atomic reference-count traffic and unclear final ownership can obscure hot paths and shutdown |
| `std::weak_ptr<T>` | Non-owning observation of shared state | Only relevant when the ownership model already requires `shared_ptr` |

`std::span` was standardized as a cheap value-type view over storage owned elsewhere, replacing an error-prone pointer-plus-length interface while decoupling algorithms from containers ([WG21 P0122R7](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0122r7.pdf)). It is not an ownership solution. The same distinction should be visible for image buffers, ray batches, queue slices, spectra, and serialized data.

Candidate enforceable rules:

- Ban owning raw pointers in new code. Construction occurs through values, `make_unique`, or an allocator/arena abstraction whose owner and lifetime region are named.
- Do not put `unique_ptr` or `shared_ptr` in a parameter merely to access an object. Pass a reference, pointer, or span unless ownership is transferred or shared.
- Mark result objects that must be inspected `[[nodiscard]]`; do not rely on comments such as “caller owns.”
- Make aliases and views visibly non-owning. A type that stores a reference, pointer, span, or string view documents the required source lifetime.
- Keep resource wrappers movable where useful and non-copyable unless copying has real semantic meaning.

The exact role of GSL `owner<T*>`/`not_null`, intrusive ownership, arena handles, and polymorphic allocators remains a project decision. `clang-tidy`'s owning-memory check is type-based and explicitly has limitations, so it is a guardrail rather than a proof ([`cppcoreguidelines-owning-memory`](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/owning-memory.html)).

## RAII beyond memory

RAII should cover files, mapped data, worker threads, locks, temporary directories, profiling scopes, GPU allocations, command streams, modules/programs, and device contexts—not just heap objects. Constructors or factories establish an invariant; destruction releases it; partially completed construction cannot leak. Locks use scoped guards, never manually paired lock/unlock calls ([Core Guidelines CP.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-raii)).

Enforceable consequences:

- Destructors and cleanup paths do not throw. A fallible flush/close operation needs an explicit method before destruction when the error matters.
- No public `init()`/`shutdown()` protocol leaves an object observably half-initialized. A factory can return a failure when construction itself is fallible.
- Every low-level API handle is immediately adopted by a small wrapper. Backend-specific destruction stays behind that wrapper.
- Per-render and per-thread scratch allocators are scoped to their real lifetime; persistent scene/device allocations have a different owner.

## Dependencies without ambient state

The Core Guidelines advise avoiding non-`const` globals and singletons, and recommend scoped objects for startup and cleanup ([I.2 globals](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-global), [I.3 singletons](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-singleton), [I.8 scoped objects](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-scoped)). For a workbench that must instantiate and compare multiple execution variants in one process, this can be made a design constraint:

- Constructors and factories receive required capabilities explicitly. Tests can supply small fakes or in-memory implementations.
- Mutable process-wide registries, singleton devices, hidden thread pools, global allocators, and global configuration are disallowed by default.
- Compile-time constants and immutable lookup tables are not the same problem; their initialization still must not depend on cross-translation-unit order.
- A scoped context may own coherent process/session resources, but it should expose capability-shaped views rather than a generic `get<T>()` service locator.
- Separate long-lived configuration/capabilities, mutable execution state, and short-lived scratch. One “everything context” makes dependencies implicit again.
- Verify the model by constructing two independent contexts with different configurations in one test and destroying them in either order.

`clang-tidy` has a direct check for non-constant globals, but it cannot determine whether a context has become a disguised service locator ([official check](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/avoid-non-const-global-variables.html)). Context granularity, reference retention, and cache ownership therefore remain reviewable design decisions.

## Constness and data-race discipline

Prefer immutable data and `const` operations, minimize explicit sharing, and treat a data race as a defect rather than an accepted performance technique ([Con.1 immutability](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rconst-immutable), [CP.2 data races](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-races), [CP.3 sharing](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-shared)). A practical discipline is:

- Published scene/configuration values are immutable. Mutable accumulation, queues, and statistics have named owners and synchronization.
- Prefer partitioning work so each worker owns its writes. Share read-only data; merge explicit results.
- A `const` member function must not silently populate an unsynchronized cache. `mutable` means “logically unobservable,” not “thread-safe.”
- Every shared mutable field has one documented synchronization strategy: guarded by a particular mutex, atomic with stated memory-order reasoning, or thread-confined.
- Lock ownership uses RAII. No callback into unknown code while holding an internal lock unless the interface promises it.
- Run a separate ThreadSanitizer configuration. TSan detects races but typically costs 5–15× runtime and 5–10× memory, so it is a CI/test profile, not a normal or benchmark build ([Clang TSan](https://clang.llvm.org/docs/ThreadSanitizer.html)).

Clang's thread-safety attributes can statically check lock capabilities when annotations are acceptable, but they are a Clang extension and should be isolated behind portable macros if adopted ([Thread Safety Analysis](https://clang.llvm.org/docs/ThreadSafetyAnalysis.html)).

## Strong semantic types

The Core Guidelines recommend precise, strongly typed interfaces and scoped enumerations; their example replaces an ambiguous `double` with a unit-bearing speed type ([I.4 typed interfaces](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-typed), [Enum.3 `enum class`](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Renum-class)). This is mechanically valuable in rendering, where many quantities share a representation.

Candidate rule: use a distinct type when accidentally interchanging two values would be a meaningful defect. Potential candidates include radians/degrees, world/object/raster coordinates, wavelength/sample/pixel counts, PDF versus throughput, device IDs, byte counts, and alignment. A strong type should have explicit construction, only meaningful arithmetic, and no implicit escape to its representation.

This rule should not produce a wrapper for every integer or float. The open decision is where semantic protection exceeds conversion and generic-programming friction, especially inside SIMD kernels. Keep semantic types at interfaces even if a measured inner loop deliberately lowers them to a documented representation.

## Error-policy options

The project needs one documented taxonomy before choosing syntax:

| Condition | Candidate expression | Trade-off to decide |
|---|---|---|
| Violated internal invariant/programmer precondition | assertion, test failure, or immediate termination | C++23 has no standard contracts facility; assertions may compile out |
| Expected recoverable failure | `std::expected<T, E>` | Explicit and composable, but propagates through signatures and requires an error vocabulary |
| Failure that prevents a high-level operation from completing | exception caught at a subsystem/application boundary | Works naturally with RAII and constructors, but control flow is implicit and may not fit device/ABI boundaries |
| Optional absence with no diagnostic | `std::optional<T>` | Cannot explain why a value is absent |
| Cancellation | dedicated stopped/cancelled outcome | Should not be confused with a renderer defect or invalid input |

`std::expected<T,E>` is a C++23 vocabulary type containing either a value or an error; its proposal explicitly compares its visibility and composition with exceptions and return codes ([WG21 P0323R12](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p0323r12.html)). The Core Guidelines generally favor exceptions for functions that cannot perform their task, RAII for cleanup, and `noexcept` only when a function cannot throw ([error-handling rules](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Re-errors)). Those recommendations do not settle this workbench's CPU/GPU and library-boundary constraints.

Whichever mix is chosen should be enforceable:

- Never use a sentinel value when it overlaps the valid domain.
- Do not ignore status silently; use `[[nodiscard]]` where meaningful.
- Convert backend/compiler/driver failures to a stable host-side diagnostic at one boundary.
- Preserve context and source location without logging the same failure at every layer.
- Mark `noexcept` from a real guarantee, not as an optimization wish.
- Test failure paths and RAII cleanup under partial construction.

Open decisions include whether exceptions are permitted in the core library, whether `expected` is limited to recoverable/domain failures, the shape of error payloads, and the boundary where CLI code turns failures into messages and exit codes.

## Compiler warnings and conformance

The portable baseline is ISO C++23 with extensions disabled. CMake's `CXX_EXTENSIONS` controls whether a compiler extension dialect such as `gnu++23` is requested, while `target_compile_features(... cxx_std_23)` expresses the language requirement ([CMake compile features](https://cmake.org/cmake/help/latest/manual/cmake-compile-features.7.html), [`CXX_EXTENSIONS`](https://cmake.org/cmake/help/latest/prop_tgt/CXX_EXTENSIONS.html)). MSVC's `/permissive-` enables stricter conformance behavior ([MSVC conformance mode](https://learn.microsoft.com/en-us/cpp/build/reference/permissive-standards-conformance?view=msvc-170)).

A candidate warning baseline for **project-owned targets** is:

| Compiler frontend | Baseline candidate | Stricter CI candidate |
|---|---|---|
| Clang/GCC | `-Wall -Wextra -Wpedantic` | `-Werror` after compiler versions are pinned |
| MSVC | `/W4 /permissive- /Zc:__cplusplus` | `/WX` after compiler versions are pinned |

GCC documents that `-Wall` and `-Wextra` are collections rather than “all warnings,” and that `-Werror` promotes enabled warnings ([GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)). Microsoft recommends `/W4` for lint-like checking in new projects and documents `/WX` separately; `/Zc:__cplusplus` makes the `__cplusplus` macro report the selected language mode rather than its historical fixed value ([MSVC warning levels](https://learn.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level?view=msvc-170), [`/Zc:__cplusplus`](https://learn.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-170)).

`-Wconversion`, `-Wsign-conversion`, `-Wshadow`, Clang `-Weverything`, and MSVC `/Wall` are policy decisions, not universal defaults. They can be useful audits but may be noisy in numerical, intrinsic, and external headers. Suppressions should be narrow and justified; do not globally weaken a warning because one backend requires a construct. Treat warnings as errors in pinned CI for project code, not automatically for third-party headers or downstream consumers; MSVC provides dedicated external-header warning controls for this purpose ([external-header diagnostics](https://learn.microsoft.com/en-us/cpp/build/reference/external-external-headers-diagnostics?view=msvc-170)). GCC also advises against combining `-Werror` with sanitizer builds because instrumentation can increase false-positive compiler warnings ([GCC instrumentation guidance](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)).

## Formatting, clang-tidy, and static analysis

- Commit one `.clang-format` and pin a compatible formatter version in CI. clang-format's official configuration mechanism is a repository file; exact brace, width, and naming choices are subjective ([clang-format options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)).
- Commit one `.clang-tidy`, generate `compile_commands.json`, and run it as a separate target/job or via CMake's target `CXX_CLANG_TIDY` property ([clang-tidy usage](https://clang.llvm.org/extra/clang-tidy/), [CMake property](https://cmake.org/cmake/help/latest/prop_tgt/LANG_CLANG_TIDY.html)).
- Select checks explicitly or pin the tool version before using broad globs: the official check inventory evolves. High-signal candidates can come from `bugprone-*`, `performance-*`, selected `cppcoreguidelines-*`, and selected `portability-*`; each must be trialed against the math/SIMD code ([official check list](https://clang.llvm.org/extra/clang-tidy/checks/list.html)).
- Keep automatic fixes reviewable. A formatter may be unconditional; semantic tidy fixes should not be applied blindly.
- Run the Clang Static Analyzer as an additional bug finder, not as proof of correctness ([Clang Static Analyzer](https://clang.llvm.org/docs/ClangStaticAnalyzer.html)). Do not assume GCC `-fanalyzer` is an equivalent C++ gate: current GCC documentation describes it as expensive, neither sound nor complete, and suitable only for C in that release ([GCC static analyzer](https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html)).

The exact tidy checks, warning allowlist, analyzer cadence, and tool-version pinning are open decisions. Naming and layout rules belong in the formatter or a very small explicit list, not mixed with correctness claims.

## Sanitizer, test, and benchmark profiles

Sanitizers are complementary configurations, not one universal switch. ASan detects out-of-bounds, use-after-free, and related memory defects; UBSan instruments selected undefined behavior; TSan detects data races. ASan and TSan cannot be combined, so TSan must be its own build ([Clang ASan](https://clang.llvm.org/docs/AddressSanitizer.html), [Clang UBSan](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html), [GCC sanitizer combinations](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)). MSVC supports AddressSanitizer on Windows x86/x64 but does not offer the same complete sanitizer matrix, so CI expectations must be compiler/platform-specific ([MSVC ASan](https://learn.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-170)).

Candidate build matrix:

| Profile | Purpose | Typical properties; exact flags remain configurable |
|---|---|---|
| Developer debug | Fast iteration and assertions | Debug info, warnings, tests, inexpensive assertions |
| ASan + UBSan | Memory/lifetime/UB tests | Clang or GCC, `-O1` or similar, debug info, frame pointers, full test suite |
| TSan | Race detection | Separate Clang/GCC build, `-O1` or higher, debug info, concurrency tests |
| Portable release | Correct optimized baseline | Optimized, no `-march=native`, tests enabled, scalar path always buildable |
| Benchmark | Measurement only | Full chosen optimization/ISA, no sanitizer/coverage/tidy instrumentation, fixed assertions policy, recorded compiler/flags/hardware |
| Static analysis | Non-runtime defect search | Compilation database, pinned tidy/analyzer configuration |

Run tests in both a checking build and an optimized build; optimizer-dependent undefined behavior and assertion-dependent behavior otherwise escape. CMake's `CTest` module supplies the conventional `BUILD_TESTING` option and test registration, and presets can name configure/build/test workflows ([CTest module](https://cmake.org/cmake/help/latest/module/CTest.html), [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)).

Benchmark results must record build type, compiler and version, dependency graph, target architecture/ISA, assertions, sanitizer state, thread count, and relevant hardware/runtime versions. Whether benchmark assertions remain enabled, whether LTO/PGO gets separate profiles, and what constitutes a stable benchmark machine are project decisions.

## Target-based CMake

CMake models a build as logical targets whose usage requirements propagate through dependencies. `PUBLIC`, `PRIVATE`, and `INTERFACE` express whether requirements affect the target, its consumers, or both ([CMake build-system manual](https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#build-specification-and-usage-requirements)). Candidate enforceable rules:

- Express C++23 with `target_compile_features`; set `CXX_EXTENSIONS OFF` on project targets.
- Use `target_sources`, `target_include_directories`, `target_compile_definitions`, `target_compile_options`, and `target_link_libraries` with explicit scope.
- Link dependencies through CMake targets, preferably namespaced imported targets, rather than raw include paths, library directories, or filenames.
- Keep warning, sanitizer, coverage, and ISA flags target-scoped. Do not mutate global `CMAKE_CXX_FLAGS`.
- Propagate only real consumer requirements. CMake explicitly warns that usage requirements are not a vehicle for forcing mere recommendations downstream.
- Apply project warnings/tidy to project code, not automatically to dependencies or consumers.
- Give scalar and each ISA-specific implementation distinct targets or tightly scoped source properties; do not leak an ISA requirement through the semantic library interface.
- Put shared checked-in workflows in `CMakePresets.json`; keep machine-local paths/settings in untracked `CMakeUserPresets.json` ([presets manual](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)).

`cxx_std_23` asks for a compiler mode at least as new as C++23; it does not prove that every desired language and standard-library facility is implemented. Required facilities should compile in every supported toolchain, with standardized feature-test macros used only where conditional support is intended ([CMake compile-feature meta-features](https://cmake.org/cmake/help/latest/manual/cmake-compile-features.7.html#requiring-language-standards), [WG21 P0941R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0941r2.html)).

Open decisions: minimum CMake version, static/shared defaults, install/export support, unity builds, modules, IPO/LTO, and whether warning/sanitizer settings use helper functions or private interface targets.

## Conan 2 integration and reproducibility

Conan's recommended CMake integration generates dependency targets with `CMakeDeps` and build settings/toolchain/presets with `CMakeToolchain`; profiles capture compiler, architecture, build type, options, environment, and tool requirements ([Conan CMake integration](https://docs.conan.io/2/integrations/cmake.html), [profiles](https://docs.conan.io/2/reference/config_files/profiles.html)).

Candidate enforceable rules:

- Let Conan describe external dependencies and toolchain settings; let CMake describe project targets. Do not duplicate dependency paths or compiler settings manually in both systems.
- Consume Conan packages via generated CMake targets.
- Keep generated files in the build tree, never in source-controlled source directories.
- Use distinct build and host profiles for cross-compilation; this prevents build-machine tools from being confused with target libraries ([Conan CMake tools and cross-building](https://docs.conan.io/2/reference/tools/cmake.html)).
- Include compiler, version, standard library, architecture, build type, and relevant options in maintained profiles/CI inputs.
- Use lockfiles when an experiment or release must reproduce exact dependency revisions; Conan documents lockfiles as fixing the resolved dependency revisions even when newer versions appear ([lockfile pipeline](https://docs.conan.io/2/ci_tutorial/packages_pipeline/multi_configuration_lockfile.html)).

Open decisions: `conanfile.py` versus `conanfile.txt`, minimum Conan version, which developer profiles are committed, lockfile update cadence, binary-cache/remotes policy, and whether tool dependencies such as clang-tidy/format are managed by Conan or the host environment.

## Compiler and architecture portability

The Core Guidelines recommend ISO C++ and isolating unavoidable extensions behind interfaces ([P.2 ISO C++](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rp-Cplusplus)). For a workbench intentionally exploring SIMD and GPUs, “portable” should mean controlled specialization rather than avoiding specialization:

- Maintain a scalar reference implementation that compiles without architecture-specific flags.
- Compile x86 and ARM intrinsic implementations in isolated targets/translation units with their own flags. Never apply `-march=native` or an equivalent to the portable library: GCC documents that `native` enables instruction subsets supported by the build machine and may produce a binary that does not run elsewhere ([GCC x86 options](https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html)).
- Centralize compile-time feature detection and runtime ISA/device selection. Do not scatter compiler/architecture `#ifdef`s through transport semantics.
- Keep compiler intrinsics, inline assembly, alignment assumptions, and device qualifiers behind narrow backend modules. Any intentional aliasing or representation trick gets a local test and comment explaining the language rule relied on.
- Test Clang and GCC on at least one Unix-like platform, MSVC on Windows, and both x86-64 and ARM64 as the project matures. Add both Debug/checking and optimized builds.
- Cross builds use explicit Conan build/host profiles and CMake toolchains. A successful cross-compile is distinct from executing the target tests.
- Avoid treating equal images as necessarily bit-identical across compiler, ISA, or floating-point contraction settings. Define numerical tolerances or statistical invariants per test class.
- Keep relaxed floating-point behavior explicit. GCC documents that `-Ofast` enables `-ffast-math` and disregards strict standards compliance; MSVC documents that `/fp:fast` can change reassociation and NaN, infinity, signed-zero, and rounding behavior. If evaluated, fast math should be a named, separately validated variant rather than an invisible release default ([GCC optimization options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html), [MSVC floating-point modes](https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170)).

Exact compiler versions, standard-library implementations, CI operating systems, baseline ISA, allowed floating-point modes, and the first ARM target are open decisions. C++23 support is per feature, not a single boolean; the official [Clang](https://clang.llvm.org/cxx_status.html), [GCC](https://gcc.gnu.org/projects/cxx-status.html), and [MSVC](https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance?view=msvc-170) status tables should be checked when selecting minimum versions. Clang's TSan documentation likewise illustrates why the matrix must be explicit: supported architectures and operating systems vary by sanitizer ([TSan platforms](https://clang.llvm.org/docs/ThreadSanitizer.html#supported-platforms)).

## Candidate gates versus open decisions

These are strong candidates for gates because they are objective and reversible:

- Build all project targets as C++23 with extensions off.
- Format check with a pinned `.clang-format`.
- Warning-clean Clang/GCC/MSVC project builds using a recorded baseline.
- No direct owning raw pointers, naked `new`/`delete`, or mutable globals without a documented localized exception.
- Tests registered with CTest; separate ASan/UBSan and TSan jobs where supported.
- Target-scoped CMake settings and Conan-generated dependency targets.
- A portable scalar build plus architecture-specific compile jobs.

These require an explicit project decision before enforcement:

1. Exceptions, `std::expected`, or a mixed error boundary.
2. Whether `shared_ptr` is allowed generally or only at named ownership boundaries.
3. GSL adoption for `not_null`, `owner`, and bounds/lifetime annotations.
4. Context shape and which services may be process-, session-, scene-, render-, or thread-scoped.
5. The list of strong semantic types exposed at public and hot-loop boundaries.
6. Aggressive conversion/shadow warnings and the initial explicit clang-tidy checks.
7. Minimum compiler, CMake, Conan, clang-format, and clang-tidy versions.
8. Sanitizer platform requirements and suppression policy.
9. Benchmark assertion, floating-point, LTO/PGO, and native-ISA policies.
10. Lockfile and dependency-update policy.

The useful standard is the one a contributor can understand from the type signatures and reproduce from named build presets. A short enforced core plus recorded exceptions will teach more—and decay less—than a long style guide whose rules are not connected to the compiler, analyzer, tests, or build graph.
