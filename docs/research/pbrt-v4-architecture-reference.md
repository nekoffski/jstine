# PBRT v4 Architecture Reference

_Source snapshot: official `mmp/pbrt-v4` repository at commit [`5f7a606`](https://github.com/mmp/pbrt-v4/tree/5f7a606806a4ac7b939131ded9d7a30ebd02416e), dated 14 June 2026, plus the official fourth-edition book and user guide. This is a reference analysis, not a proposed architecture._

## Executive summary

PBRT v4 is best understood as a rendering application whose implementation is factored into a static library. Its executable performs option parsing, global initialization, scene parsing, and a top-level choice between `RenderCPU()` and `RenderWavefront()`; almost all substantive code lives in `pbrt_lib`. CMake installs that library but does not install a public header tree or exported package configuration, so the library target is an internal build boundary rather than a supported SDK contract ([CMake targets](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/CMakeLists.txt), [`pbrt.cpp`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/cmd/pbrt.cpp)).

Architecturally, PBRT has two rendering organizations:

- CPU-only integrators use a small virtual `Integrator` hierarchy. Most camera-path algorithms implement a per-ray `Li()` method and inherit tile scheduling, camera-ray generation, film accumulation, and thread-local resources.
- CPU/GPU wavefront rendering uses a separate, non-derived `WavefrontPathIntegrator`. It restates volumetric path tracing as explicit queues and kernels, with only intersection hidden behind `WavefrontAggregate`.

PBRT shares most camera, film, sampler, material, light, medium, spectrum, and sampling implementations across CPU and CUDA by combining tagged-pointer dispatch, `PBRT_CPU_GPU` functions, unified memory, and compile-time specialization. Geometry acceleration and image textures have device-specific representations. This achieved impressive source reuse for one CPU path and one CUDA/OptiX path, but it is not a general runtime backend system ([system overview](https://www.pbr-book.org/4ed/Introduction/pbrt_System_Overview), [GPU overview](https://www.pbr-book.org/4ed/Wavefront_Rendering_on_GPUs)).

## Actual application and build shape

```text
pbrt CLI
  ├─ parse PBRTOptions; InitPBRT()
  ├─ ParseFiles(ParserTarget)
  │    └─ BasicSceneBuilder → BasicScene
  └─ choose once
       ├─ RenderCPU(BasicScene)
       │    └─ CPU Integrator hierarchy
       └─ RenderWavefront(BasicScene)
            └─ WavefrontPathIntegrator on CPU or CUDA/OptiX

pbrt_lib (static)
  ├─ core rendering types and utilities
  ├─ parser / scene construction
  ├─ CPU integrators and accelerators
  ├─ wavefront pipeline
  └─ optional CUDA / OptiX implementation
```

The command executable is thin in terms of rendering logic but owns process concerns: command-line validation, selection of rendering coordinate space, `InitPBRT()`, format/upgrade modes, and final `CleanupPBRT()`. Backend selection is a branch on global options: GPU or `--wavefront` calls `RenderWavefront(scene)`; otherwise it calls `RenderCPU(scene)` ([CLI main](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/cmd/pbrt.cpp)). Tools such as `imgtool`, `pspec`, and `plytool`, and the `pbrt_test` executable, link the same static library ([CMake](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/CMakeLists.txt)).

**Interpretation for a workbench:** separating process entry points from rendering code is worth retaining. PBRT does not, however, demonstrate a stable external library API: no small public include surface, versioned ABI, backend enumeration contract, or render-request/result object is installed. Its `Render()` operations consume constructor state and global `Options`, mutate a `Film`, and write output as a side effect.

## Scene parsing and construction

PBRT's scene path has three named stages ([scene-processing overview](https://www.pbr-book.org/4ed/Processing_the_Scene_Description)):

1. The tokenizer/parser turns each statement into a method call on `ParserTarget`.
2. `BasicSceneBuilder : ParserTarget` tracks graphics state—current transforms, material, media, area light, color space, and attributes—and emits typed scene entities.
3. `BasicScene` stores those entities and creates render-time objects.

`ParserTarget` is a useful separation. `BasicSceneBuilder` builds a scene, while `FormattingParserTarget` can reformat, upgrade, or transform scene files without constructing a renderer ([tokenizing and parsing](https://pbr-book.org/4ed/Processing_the_Scene_Description/Tokenizing_and_Parsing), [`parser.h`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/parser.h)). Imports can be parsed concurrently: a builder copies the current graphics state, parses independently, then merges buffered shapes and instance uses in deterministic order ([scene builder](https://pbr-book.org/4ed/Processing_the_Scene_Description/Managing_the_Scene_Description)).

The `*SceneEntity` values retain a type name, parameter dictionary, transforms, relationships, and source location. This delays many concrete choices and preserves useful diagnostics. Once `WorldBegin` fixes the options block, the builder passes filter, film, camera, sampler, integrator, and accelerator descriptions together to `BasicScene::SetOptions()` ([`scene.h`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/scene.h), [`scene.cpp`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/scene.cpp)).

`BasicScene` is not a pure immutable scene IR. It immediately creates filter and film, launches asynchronous jobs for camera, sampler, media, textures, lights, and image loading, and stores both high-level entities and created runtime handles. Its allocator is chosen using global `Options->useGPU`; GPU parsing/building therefore already allocates some objects in CUDA managed memory. `RenderCPU()` and the wavefront constructor later call `CreateMedia()`, `CreateTextures()`, `CreateLights()`, `CreateMaterials()`, and their own aggregate construction path ([final object creation](https://pbr-book.org/4ed/Processing_the_Scene_Description/BasicScene_and_Final_Object_Creation), [`RenderCPU`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/cpu/render.cpp), [wavefront constructor](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/wavefront/integrator.cpp)).

### Device-independent versus device-specific data

PBRT shares the parsed entities and most semantic object implementations, but it does not keep a strict “one immutable host scene, many compiled device scenes” split.

| Data | CPU path | Wavefront GPU path |
|---|---|---|
| Parsed declarations | Shared `BasicScene` / `*SceneEntity` representation | Same representation |
| Camera, film, sampler, materials, lights, media | PBRT tagged-pointer objects in host allocation | Mostly the same classes, allocated in managed memory and callable from device code |
| Geometry and acceleration | `Shape` + `Primitive`; BVH or kd-tree returned as a `Primitive` | `OptiXAggregate`; native triangles go to OptiX, other shapes use custom intersection support |
| Image textures | CPU image texture implementations | GPU-specific texture implementations using GPU texture hardware |
| Scheduling state | CPU integrator object and thread-local state | SOA pixel state and bounded work queues in managed memory |

The official GPU overview explicitly names unified addressing, C++17 device compilation, and a ray-intersection API as platform requirements. It also notes that the platform-specific CUDA/OptiX code remains necessary even though most semantic classes are shared ([GPU chapter](https://www.pbr-book.org/4ed/Wavefront_Rendering_on_GPUs), [mapping path tracing to the GPU](https://www.pbr-book.org/4ed/Wavefront_Rendering_on_GPUs/Mapping_Path_Tracing_to_the_GPU)).

**Transferable pattern:** preserve source locations and parameters in an intermediate scene, and make backend preparation an explicit step. **Caution:** PBRT's intermediate is stateful, asynchronously materializing, and allocator/backend-aware. That is efficient for “parse once, render once,” but it is not evidence that the same instance can safely or clearly compile several runtime-swappable variants.

## CPU integrator hierarchy and exact method roles

The hierarchy is small and intentionally makes the common case easy ([`integrators.h`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/cpu/integrators.h), [system overview](https://www.pbr-book.org/4ed/Introduction/pbrt_System_Overview)).

### `Integrator`

`Integrator` is the CPU hierarchy's virtual root. It owns the top-level `Primitive aggregate`, all lights, and a filtered list of infinite lights. Its constructor preprocesses lights using scene bounds. Its interface consists of:

- pure virtual `void Render()` for complete algorithm orchestration;
- `Intersect()` and `IntersectP()` forwarding to the aggregate;
- `Unoccluded()` for visibility;
- `Tr()` for transmittance between interactions.

Integrator selection is a centralized string-to-constructor `if` chain returning `std::unique_ptr<Integrator>` ([factory source](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/cpu/integrators.cpp)). This is the main exception to PBRT's tagged-pointer approach because these integrators only execute on the CPU ([dynamic-dispatch discussion](https://www.pbr-book.org/4ed/Introduction/Using_and_Understanding_the_Code)).

### `ImageTileIntegrator::Render()`

`ImageTileIntegrator` adds `Camera`, a prototype `Sampler`, and a pure `EvaluatePixelSample()`. Its concrete `Render()` owns the CPU image loop:

- derive pixel bounds and samples per pixel;
- create per-thread `ScratchBuffer` and cloned `Sampler` objects;
- render tiles with `ParallelFor2D()`;
- advance samples in progressively larger “waves”;
- reset scratch storage after each sample;
- handle progress, debug-start mode, optional live display, reference-image MSE, partial writes, metadata, and final output.

Thus `Render()` is not transport semantics; it is CPU scheduling plus experiment/UI/output policy around the per-pixel hook ([implementation](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/cpu/integrators.cpp), [book walkthrough](https://www.pbr-book.org/4ed/Introduction/pbrt_System_Overview)).

### `RayIntegrator::EvaluatePixelSample()` and `Li()`

`RayIntegrator` finalizes `EvaluatePixelSample()` so camera-path integrators do not reimplement plumbing. For one pixel/sample it:

1. samples wavelengths from the film;
2. obtains filtered film, lens, and time samples from the sampler;
3. asks the camera for a ray differential;
4. scales ray differentials for the sampling rate;
5. calls the subclass's pure virtual `Li(ray, wavelengths, sampler, scratch, visibleSurface)`;
6. diagnoses NaN and infinite radiance;
7. sends the weighted result and optional visible-surface AOV data to `Film::AddSample()`.

`Li()` is therefore a narrow **per-camera-ray radiance estimator**, not the renderer interface. `RandomWalkIntegrator`, `SimplePathIntegrator`, `PathIntegrator`, `SimpleVolPathIntegrator`, `VolPathIntegrator`, AO, and BDPT provide it. It is excellent for expressing sequential scalar algorithms, because scheduling, camera, and output details disappear from their view ([`RayIntegrator` implementation](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/cpu/integrators.cpp)).

The hierarchy's exceptions reveal its boundary. `LightPathIntegrator` inherits `ImageTileIntegrator` directly because it starts paths at lights and splats to film. BDPT supplies `Li()` but also overrides `Render()` for strategy-weight films. MLT and SPPM derive directly from `Integrator` and own their complete scheduling. The hierarchy is a convenience ladder, not a uniform model of every integration algorithm.

### `WavefrontPathIntegrator`

`WavefrontPathIntegrator` does **not** derive from `Integrator`; `RenderWavefront()` constructs it directly, calls `Float Render()`, gathers its statistics, and writes its public film ([`wavefront.cpp`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/wavefront/wavefront.cpp)). It always implements semantics equivalent to CPU `VolPathIntegrator`; other selected integrator names are ignored with a warning ([wavefront source](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/wavefront/integrator.cpp)).

Its `Render()` loops over sample indices, bounded scanline batches, and ray depth. At each depth it resets the next queues, generates sampler dimensions, traces closest hits, samples media, handles escaped and emissive rays, evaluates materials/BSDFs, traces shadow rays, processes subsurface work, and finally updates film. Two ray queues are double-buffered by depth. Queues for media, infinite misses, area-light hits, basic/universal material evaluation, shadow rays, and subsurface work are allocated only if scene features require them ([path tracer implementation](https://www.pbr-book.org/4ed/Wavefront_Rendering_on_GPUs/Path_Tracer_Implementation)).

This is a manual restructuring of the same transport algorithm, not a call to `VolPathIntegrator::Li()`. The source makes the duplication legible, but it demonstrates why a per-ray `Li()` seam cannot itself abstract scalar, SIMD-stream, and GPU-wavefront execution.

## Component seams

PBRT uses small value-like handles over concrete implementations for most renderer concepts. These handles derive from `TaggedPointer`, which stores a pointer plus a compile-time type tag and dispatches through generated `switch` logic rather than virtual calls ([tagged pointers](https://pbr-book.org/4ed/Utilities/Containers_and_Memory_Management)).

| Concept | PBRT seam and responsibility | Architectural observation |
|---|---|---|
| `Sampler` | `StartPixelSample(pixel, index, dimension)`, `Get1D()`, `Get2D()`, `GetPixel2D()`, `SamplesPerPixel()`, `Clone()` ([interface](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/base/sampler.h)) | A clear deterministic protocol; clone supplies independent mutable CPU state. Dimension consumption remains implicit in call order. |
| `Camera` | Generates rays/differentials, evaluates importance and PDFs for bidirectional methods, samples time, exposes camera transform and its `Film` ([interface](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/base/camera.h)) | Physically complete seam, but camera→film ownership/access couples sensing and output storage. |
| `Film` | Samples wavelengths, accumulates samples/splats, exposes bounds/filter/sensor, produces/writes images and metadata ([interface](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/base/film.h)) | Centralizes spectral sensor conversion and accumulation. File writing in the same interface mixes computation with application I/O. |
| `Material` | Given texture evaluator, material context, wavelengths, and scratch arena, returns `BSDF`/`BSSRDF`; exposes normal/displacement and capability queries ([interface](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/base/material.h)) | Keeps material description separate from ephemeral scattering objects; texture-evaluator templating enables specialized kernels. Closed tag lists make extension invasive. |
| `Light` | Incident/emitted sampling and PDFs, emitted radiance, power, bounds, preprocessing ([interface](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/base/light.h)) | Sampling and density stay adjacent, which supports MIS correctness. |
| `Shape` | Geometric bounds, intersection, area, and surface sampling/PDF only ([interface](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/base/shape.h)) | A strong deep seam: geometry is independent of material, emission, and medium. |
| `Primitive` | Attaches shape, material, area light, alpha, and medium; transformed primitives and CPU aggregates implement the same bounds/intersection interface ([interface](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/cpu/primitive.h)) | Composition avoids bloating shapes; making aggregates substitutable for primitives is elegant for CPU traversal. |
| `WavefrontAggregate` | Batch closest/shadow/transmittance/random intersections, routing results directly into downstream queues ([interface](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/wavefront/integrator.h)) | This is PBRT's clearest CPU-versus-OptiX execution seam, but it is shaped specifically around one wavefront integrator's queues. |

Factories use explicit string selection and `ParameterDictionary`, then report unused parameters. This keeps construction discoverable and catches misspellings, but adding a new tagged type requires changing the handle's compile-time type list, factory, dispatch-capable code, and often wavefront material queues ([sampler factory](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/samplers.cpp), [material factory](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/materials.cpp)).

## CPU parallelism and GPU wavefront organization

`InitPBRT()` creates one persistent thread pool, normally sized to available cores. `ParallelFor()` and `ParallelFor2D()` submit bounded jobs; the submitting thread helps execute them. `ThreadLocal<T>` supplies per-thread samplers, scratch arenas, allocators, and statistics without putting thread management into transport algorithms. `AsyncJob` overlaps scene parsing with object and asset creation ([parallelism](https://www.pbr-book.org/4ed/Utilities/Parallelism), [`pbrt.cpp`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/pbrt.cpp)).

The wavefront path uses SOA work items generated from `workitems.soa`. `WorkQueue<T>` adds an atomic size to an SOA allocation; kernels atomically append work, and `ForAllQueued()` chooses CPU `ParallelFor()` or a GPU launch through global `Options->useGPU` ([SOA declarations](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/wavefront/workitems.soa), [queue implementation](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/wavefront/workqueue.h), [implementation foundations](https://www.pbr-book.org/4ed/Wavefront_Rendering_on_GPUs/Implementation_Foundations)). Material work is split by concrete material type and by “basic” versus universal texture evaluation so each kernel is specialized and coherent.

The CPU can run the same wavefront path for debugging, but the book states it is normally slower than CPU `VolPathIntegrator`. It explicitly suggests SIMD consumption of multiple queued items as an exercise rather than an implemented backend ([GPU chapter and exercises](https://www.pbr-book.org/4ed/Wavefront_Rendering_on_GPUs/Exercises)). That makes PBRT valuable evidence for queue semantics, not a finished scalar/SIMD/CUDA backend family.

## Memory ownership and allocation

PBRT defines `Allocator` as a polymorphic byte allocator and threads it through factories. CPU code normally uses new/delete-backed allocation. Scene building uses per-thread monotonic arenas; short-lived BSDFs, BSSRDFs, and medium iterators use resettable per-thread `ScratchBuffer`s. GPU mode switches the upstream resource to tracked CUDA managed memory, and the wavefront integrator prefetches allocations before launching work ([memory management](https://pbr-book.org/4ed/Utilities/Containers_and_Memory_Management), [`gpu/memory.h`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/gpu/memory.h)).

This provides allocation policy without templating every semantic type and makes CPU initialization of GPU-visible pointer graphs possible. Ownership is nevertheless often implicit: tagged handles are non-owning pointers; large groups of objects live for the render/process lifetime; arena allocation bypasses individual destruction; and initialization intentionally leaks a GPU monotonic resource so its data remains alive ([initialization source](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/pbrt.cpp)).

**Transferable pattern:** separate persistent scene allocation from reset-per-sample scratch allocation. **Caution:** explicit lifetime regions and ownership documentation would be especially important in a long-lived, repeatedly recompiled, runtime-swappable workbench; PBRT's process-lifetime assumptions should not be copied accidentally.

## Runtime selection and dispatch

PBRT has several different mechanisms, each serving a narrow purpose:

- CLI/global option chooses CPU integrators versus the wavefront route.
- The CPU integrator factory uses virtual dispatch after centralized string construction.
- Camera, sampler, film, material, light, texture, medium, shape, and primitive use closed-set `TaggedPointer` dispatch.
- Wavefront host code examines tags and launches templates specialized for concrete sampler, material, texture-evaluator, or phase-function types.
- Compile-time macros include/exclude CUDA code and mark functions/lambdas for host/device compilation.
- `WavefrontAggregate` uses classic virtual dispatch on the host to choose CPU or OptiX batch intersection.

Tagged pointers avoid per-object vtable storage and the CPU/GPU virtual-table problem, but the official book explicitly notes that all possible types must be known at compile time and runtime plugins are precluded ([tagged-pointer implementation](https://pbr-book.org/4ed/Utilities/Containers_and_Memory_Management), [dispatch rationale](https://www.pbr-book.org/4ed/Introduction/Using_and_Understanding_the_Code)). The global `useGPU` flag is checked inside launch helpers and allocation paths rather than represented by an explicit backend object. This is simple for PBRT's two routes, but multiplying it across scalar, several SIMD ISAs, CUDA, and OpenCL would spread backend conditionals and compile-time type closure widely.

## Testing and validation

PBRT's strongest testing patterns are numerical and statistical rather than snapshot-heavy:

- dedicated tests cover vector math, transforms, floating-point intervals, spectra/color, RNG/sampling, containers, parser, shapes, samplers, filters, media, lights/light samplers, BSDFs, and CPU integrators;
- shape tests generate adversarial/random geometry and rays;
- BSDF tests compare sampled distributions with numerically integrated PDFs using chi-square tests;
- integrator tests construct small analytic scenes and check average radiance across samplers and integrators;
- `--debugstart` reproduces a pixel/sample, while the CPU tile renderer can emit MSE versus spp against a reference image and per-pixel statistics ([test target](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/CMakeLists.txt), [`bsdfs_test.cpp`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/bsdfs_test.cpp), [`integrators_test.cpp`](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/cpu/integrators_test.cpp), [CLI options](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/cmd/pbrt.cpp)).

CPU CI builds Linux with several GCC/Clang versions and macOS/Windows with normal, debug-logging, and double-precision configurations; the unit test executable runs for the default configuration. GPU CI spans CUDA/OptiX and operating-system combinations but is build-only ([official workflows](https://github.com/mmp/pbrt-v4/tree/5f7a606806a4ac7b939131ded9d7a30ebd02416e/.github/workflows)). Several diagnostic/output features supported by `ImageTileIntegrator` are explicitly rejected by the wavefront path, including MSE-reference and pixel-stat modes ([wavefront constructor](https://github.com/mmp/pbrt-v4/blob/5f7a606806a4ac7b939131ded9d7a30ebd02416e/src/pbrt/wavefront/integrator.cpp)).

**Transferable pattern:** combine analytic/property/statistical component tests with reproducible sample-level debugging and convergence measurements. **Gap not to copy:** build-only GPU CI and path-specific validation features do not establish cross-backend numerical equivalence.

## Patterns worth carrying forward

These patterns align well with a long-term educational workbench, without determining its final interfaces:

1. **A thin executable over implementation code.** It permits CLI-first operation while keeping experiments callable from tests and future frontends.
2. **Parser target separate from scene meaning.** Importers, formatters, and programmatic scene builders can feed the same semantic representation.
3. **Parsed entities before prepared render data.** Preserve source locations and validate parameters before device layout decisions.
4. **Shape / primitive / aggregate separation.** Geometry, binding, transforms, and acceleration remain individually understandable and testable.
5. **Sampling operations adjacent to their PDFs.** This is essential for MIS correctness across lights, BSDFs, shapes, and cameras.
6. **Prototype sampler plus isolated execution state.** PBRT's clone/thread-local approach is clear for CPU; the underlying deterministic protocol can be shared more broadly.
7. **Persistent data versus scratch arenas.** Per-sample scattering objects should be cheap and lifetime-bounded.
8. **Explicit work-item schemas and typed queues.** SOA generation makes data movement visible and measurable; scene-feature detection avoids empty subsystems.
9. **A CPU execution route for GPU queue logic.** Even if slow, it materially improves debugging and validation.
10. **Small analytic and statistical tests.** They catch estimator defects that attractive reference images hide.

## Patterns to treat as PBRT-specific, not defaults

These choices solve PBRT's goals but conflict with some combination of runtime-swappable variants, incremental development, own-SIMD learning, or API clarity:

1. **`Li()` as the universal-looking abstraction.** It is only the sequential camera-ray hook; the wavefront implementation must restate `VolPathIntegrator` outside the hierarchy.
2. **A separate wavefront integrator that is not an `Integrator`.** Different return types, output ownership, feature support, and construction paths make backend comparison less uniform.
3. **Global `Options` in allocation, launch, and algorithms.** Dependencies and capabilities are harder to see, instantiate twice, or test in one process.
4. **Closed tagged-pointer lists as the main extensibility mechanism.** They are fast and GPU-friendly, but adding an experiment touches central type lists/factories and cannot be a runtime plugin.
5. **CUDA host/device source sharing as the portability boundary.** `PBRT_CPU_GPU`, unified-memory pointer graphs, CUDA lambdas, and GPU-specific `pstd` machinery do not naturally extend to OpenCL or independently optimized SIMD layouts.
6. **Backend-aware `BasicScene` allocation.** It weakens the parsed-scene/prepared-scene boundary and assumes one backend choice before parsing completes.
7. **Implicit arena/process lifetime.** It is expedient for a batch renderer but risky for repeated scene reloads, hot backend swaps, and long interactive sessions.
8. **Camera-owned film and film-owned file output.** Convenient for the book's renderer, but it combines sensor semantics, accumulation storage, and application I/O.
9. **A monolithic static implementation library presented as a library boundary.** Without a deliberately small public API, it does not by itself answer how external tools should embed or extend the renderer.
10. **Early asynchronous scene construction.** It improves startup but adds mutexes, futures, and hybrid state before basic scene compilation is conceptually settled.

## Questions PBRT usefully leaves open

PBRT's source suggests concrete design questions for the workbench rather than answers to adopt wholesale:

- What is the smallest transport-semantic contract shared by sequential `Li`, CPU stream/SIMD, and GPU wavefront implementations without pretending their scheduling is identical?
- Should a parsed scene be immutable and reusable, with each backend producing a separately owned prepared scene?
- Which concepts need open runtime registration for experiments, and which benefit from closed compile-time dispatch inside performance kernels?
- Where should sampler dimensions become explicit enough for scalar/SIMD/GPU differential testing?
- Is accumulation a shared semantic service while storage and file output remain frontend concerns?
- What lifetime owns device buffers, compiled acceleration, scratch memory, and scene reloads?
- Which validation hooks must be common to every backend from its first milestone?

The most valuable PBRT lesson is the separation between **transport semantics** and **execution dataflow**, even though its own architecture does not expose that as one general interface. Its CPU `Li()` implementations are exceptionally readable; its wavefront queues are exceptionally explicit. A research workbench can learn from both while preserving the freedom to give scalar, SIMD, CUDA, and OpenCL implementations distinct internal organizations.
