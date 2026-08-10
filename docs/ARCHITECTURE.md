# Architecture

## Purpose

`jstine` is a rendering research workbench, not a production renderer or a compatibility clone of PBRT. Its priorities, in order, are:

1. Understandability and correctness.
2. Reproducible experimentation.
3. Measured performance.
4. Feature breadth and production polish.

The code should make rendering algorithms and execution trade-offs visible. Early implementations remain useful as references; optimized implementations are added beside them rather than replacing them.

## Scope and constraints

- Modern C++ with CMake and Conan.
- A shared library linked by a thin CLI and benchmark executables. The long-term SDK and ABI contract is intentionally undecided.
- CPU execution first: scalar, multithreaded scalar, then explicit SIMD.
- GPU compute later. The concrete API is a future decision; the core does not assume Vulkan, Metal, or hardware ray traversal.
- PBRT v4 supplies the primary vocabulary, theoretical progression, and many implementation patterns. Compatibility with PBRT source or scene files is not a goal.
- The project owner writes the implementation code. Dependencies may be used when appropriate and are chosen during implementation.

## Architectural principles

### Preserve references; add specializations

The scalar renderer is a permanent correctness oracle. Brute-force intersection remains available after BVHs arrive. Strict floating-point implementations remain available beside relaxed experiments. SIMD and GPU implementations must be compared with references rather than silently replacing them.

### Share semantics, specialize execution

The Scene Model, spectral and sampling semantics, render requests, and observable results are shared. Scene layout, acceleration representation, scratch state, scheduling, SIMD organization, and device kernels belong to a concrete execution implementation.

There is no initial universal low-level backend interface. A scalar `Li()` loop, a SIMD packet or stream implementation, and a GPU compute pipeline organize work differently. A reusable execution seam is extracted only after multiple concrete implementations demonstrate it.

### Keep dependencies and lifetimes explicit

There are no mutable singletons, global options, global schedulers, global RNGs, or generic service locators. A composition root constructs long-lived facilities. Render Sessions own per-render state. Scoped contexts carry genuinely cross-cutting session facilities through orchestration code, while hot kernels receive narrow typed arguments.

### Keep runtime polymorphism cold

Virtual dispatch is acceptable at coarse, configured seams such as Integrator adapters and Render Sessions. Math, scene data, intersections, sampling, BSDF evaluation, and traversal use values, closed tagged alternatives, or implementation-local templates and concepts. A scene is not an inheritance tree.

### Make experiments reproducible

A Sample Address consists of pixel, sample index, dimension, and experiment seed. It does not depend on thread scheduling or queue order. Scalar results should be repeatable across thread counts when accumulation order is fixed. SIMD and device implementations need numerical or statistical equivalence, not bit identity.

## C++ design contract

These are architecture rules, independent of formatter, linter, sanitizer, and CI choices:

| Concern | Required shape | Avoid |
|---|---|---|
| Dependencies | Composition-root ownership, narrow constructor injection, scoped typed contexts | Singletons, mutable globals, hidden construction, `get<T>()` service lookup |
| Ownership | Values first, `std::unique_ptr` for exclusive ownership, explicit non-owning references and spans | Owning raw pointers, manual `new`/`delete`, reflexive `std::shared_ptr` |
| Errors | `Result<T>` for recoverable seams, assertions/contracts for internal invariants, dependency translation in adapters | Sentinel errors, ignored status, exceptions crossing worker/device/C seams |
| State | Immutable published scene and configuration; named owners for queues, accumulation, caches, and statistics | Unsynchronized lazy mutation, ambient caches, unclear teardown order |
| Polymorphism | Virtual dispatch at cold configured seams; values, tags, and localized templates in kernels | Inheritance-based scene entities and per-ray/per-BSDF virtual dispatch |
| Types | Distinct points, vectors, normals, spectra, IDs, and other defect-preventing semantic types | Reusing `Vec3` or primitive integers for unrelated meanings |
| Module interfaces | Small typed surfaces that hide preparation, storage, scheduling, and resource management | Pass-through wrappers, third-party types leaking across core seams, cyclic dependencies |

Context parameters are a mechanism for coherent per-session capabilities, not a substitute for designing parameters. If only one or two dependencies are needed, pass them directly. A context must not be retained beyond its documented owner.

## System shape

```text
CLI / benchmarks / future viewer
              |
              v
          Renderer
              |
        startup selection
     (algorithm + execution)
              |
              v
  backend-specific Integrator adapter
      |                       |
      v                       v
shared Scene Model      private Prepared Scene
                              |
                              v
                         Render Session
                         /      |      \
                   progress  snapshot  metrics
                              |
                              v
                    OpenEXR / PNG adapters
```

Selection happens once when configuring or starting a render. Implementations are not replaced while a Render Session is active.

## Core modules

### Foundation

Provides `Result<T>`, structured errors, strong IDs, non-owning views, and small lifetime utilities. `Result<T>` is a thin project abstraction based on C++23 `std::expected`. Recoverable boundary failures use `Result`; internal invariant failures use assertions or contract-style checks. Dependency exceptions are translated at adapters and never cross worker, C, or device seams.

### Math

Owns scalar semantic types such as vectors, points, normals, matrices, transforms, rays, and bounds. Points, vectors, normals, spectra, and display RGB remain distinct even if their storage is similar. Coordinate conventions, ray intervals, normal transforms, and numerical policies are explicit and independently tested.

### SIMD

Owns narrow pack, mask, and batch types used by measured rendering kernels. It is separate from scalar geometry math: a three-component vector is not a SIMD packet. The module begins with scalar and ARM NEON adapters, grows from renderer needs, and may later add AVX2 or AVX-512. ISA-specific compilation stays local to adapters.

### Spectrum and color

Owns wavelength-dependent scene spectra, sampled wavelengths with their PDFs, Transport Spectra, sensor response, and conversion to image color spaces. Transport is sampled spectral from the baseline. RGB is allowed for authored inputs and display/output, not as the representation of transported light.

### Sampling

Owns Sample Addresses, deterministic sample generation, one- and two-dimensional distributions, and later low-discrepancy or progressive sequences. Sampling interfaces keep the relationship between generated values and their probability densities explicit. The same addressing semantics apply across scalar, SIMD, and device implementations.

### Scene

The Scene Model is an immutable, device-independent description of geometry, instances, materials, textures, lights, media, and cameras. It preserves stable IDs and useful source diagnostics without containing device buffers, BVH nodes, work queues, or scheduler state.

Tiny validation scenes are constructed programmatically first. Importers are adapters that populate the same model. glTF 2.0 is the first planned external asset format; PBRT scene import remains optional.

### Geometry and acceleration

Following PBRT's useful split:

- A shape describes geometric bounds, intersection, area sampling, and the matching PDFs.
- A primitive binds geometry to material, emission, medium, and transforms.
- An aggregate accelerates intersection over primitives.

The brute-force aggregate and each BVH implementation remain selectable for tests and benchmarks. Backend-specific prepared scenes may use different node formats, layouts, and traversal kernels.

### Transport

Owns interactions, BSDFs, phase functions, Fresnel models, lights, emitted and incident sampling, and their PDFs. Sampling and density evaluation stay adjacent so that MIS behavior can be tested locally. Materials describe how to construct ephemeral scattering state; they do not form a virtual entity hierarchy in hot paths.

### Film and image

Film owns sensor-aware spectral accumulation, filters, AOV accumulation, and safe snapshots. It does not own filenames or write files. OpenEXR is the canonical linear artifact with floating-point channels and run metadata. PNG is a tone-mapped preview. Raw, un-denoised output remains available for validation.

### Renderer

Renderer is the front door used by the CLI, benchmarks, tests, and a future viewer. It validates a render request, selects a configured Integrator adapter once, prepares the session, and returns a Render Session. Process concerns such as argument parsing and file naming stay in frontends.

A render request describes at least:

- Scene and camera selection.
- Integrator algorithm and execution implementation.
- Image extent, crop, sample budget, and seed.
- Integrator parameters such as depth and Russian-roulette policy.
- Requested outputs and strictness or diagnostic options.

Configuration is typed after parsing; unstructured option maps do not flow through the rendering core.

### Integrator adapters

An Integrator expresses a light-transport algorithm, but each executable adapter is bound to an execution organization. Initial examples are:

- `ScalarPathIntegrator`: PBRT-style camera-path implementation with a readable `Li()` estimator.
- `SimdPathIntegrator`: packet or stream implementation with explicit masks, layouts, and compaction.
- A future device path integrator: compute kernels organized initially as a megakernel and later as a wavefront experiment.

The common seam controls a complete render; `Li()` is a specialization for compatible sequential ray integrators, not the universal renderer interface. A factory may begin as a simple configuration switch. A registry or plugin ABI is not required.

### Render Session

A Render Session is progressive, cancellable, and immutable after it starts. Its interface exposes progress, safe snapshots, statistics, diagnostics, cancellation, and completion. Pause/resume, live scene mutation, backend replacement, distributed rendering, and hot integrator changes are outside the initial contract.

The session owns or keeps alive:

- The immutable scene snapshot used for the render.
- The adapter-specific Prepared Scene.
- Persistent scene and device allocations.
- Resettable per-worker or per-path scratch storage.
- Scheduling and accumulation state.
- Cancellation and measurement state.

### Import and output adapters

glTF input, OpenEXR output, PNG previews, configuration parsing, and future UI integration live outside rendering semantics. Third-party failures are translated into project errors here. These adapters may depend on external libraries; the rendering core does not depend on their concrete formats.

## Ownership and context model

The default ownership order is value, then `std::unique_ptr` for exclusive dynamic ownership. References, pointers, and `std::span` are non-owning. `std::shared_ptr` is reserved for genuine simultaneous ownership. Raw owning pointers and manual `new`/`delete` do not appear in module interfaces.

```text
composition root
  owns long-lived scheduler/diagnostic facilities
        |
        v
Renderer dependencies (narrow constructor injection)
        |
        v
Render Session (per-render ownership)
        |
        +-- Render Context: cancellation, diagnostics, metrics,
        |                   scheduler, session memory
        |
        +-- specialized hot-path views: rays, scene arrays,
                                     queues, scratch, film tiles
```

A context has a named lifetime and a fixed typed surface. It never provides `get<T>()`, hidden construction, optional grab-bag services, or access to process-global state.

Persistent prepared-scene allocations and resettable sample/path scratch allocations are distinct. Device handles and buffers use backend-specific RAII wrappers and die with the Prepared Scene or Render Session that owns them.

## Dependency direction

The intended dependency direction is:

```text
frontends and adapters
        |
        v
renderer and sessions
        |
        v
integrator adapters and prepared scenes
        |
        v
transport / acceleration / scene
        |
        v
spectrum / sampling / SIMD / math / foundation
```

Lower layers do not access configuration globals, frontends, file formats, or output paths. CPU- and device-specific modules depend inward on shared semantics; shared semantics do not include device headers.

## PBRT alignment and deliberate differences

Patterns retained from PBRT v4:

- Thin executables over implementation code.
- Parsed scene meaning separated from prepared rendering data.
- Shape, primitive, and aggregate separation.
- A whole-render Integrator seam with a `Li()` convenience layer for sequential camera paths.
- Sampling operations beside their PDFs.
- Persistent allocations separated from resettable scratch memory.
- Typed work queues for bulk execution.
- Analytic, property, statistical, and reproducibility-oriented tests.

Deliberate differences:

- Renderer and Render Session provide a uniform frontend contract.
- The Scene Model remains backend-independent and reusable.
- No global options, allocators, scheduler, or device switch.
- Concrete integrator adapters bind algorithm and execution without pretending every pair is composable.
- SIMD and future device layouts may differ from scalar layout.
- CUDA host/device source sharing and managed memory are not assumed by the core.
- Film does not write files.
- Runtime plugins and a stable external ABI are deferred.

## Validation contract

Validation is layered:

- Analytic and property tests for math, transforms, intersections, spectra, and distributions.
- Differential tests for brute force versus BVH and scalar versus SIMD kernels.
- Statistical tests for samplers, BSDF/PDF consistency, energy behavior, and estimator convergence.
- Reproducible path inspection identified by pixel, sample, dimension, and seed.
- Image comparisons against high-sample references and independent implementations.
- Performance comparisons that state scene, device, compiler, flags, sample budget, error, and memory behavior.

A plausible image is not sufficient correctness evidence. A denoised image is never used to validate transport.

## Explicitly deferred decisions

- Stable installed SDK, public header, and ABI policy.
- First GPU compute API and target hardware.
- Exact dependency choices for import, output, tests, benchmarks, and UI.
- Long-term material interchange beyond the first glTF adapter.
- Plugin or dynamic registration model.
- Distributed rendering and live scene editing.
- Which advanced research branch follows the CPU/GPU foundations.

## Related decisions and research

- [ADRs](adr/)
- [Modern path-tracing research roadmap](research/modern-path-tracing-roadmap.md)
- [PBRT v4 architecture reference](research/pbrt-v4-architecture-reference.md)
- [Modern C++ engineering standards](research/modern-cpp-engineering-standards.md)
