# Implementation Plan

## How to use this plan

This is a dependency order, not a calendar. Each milestone leaves working code integrated through the shared library and CLI. Earlier reference implementations stay available when optimized variants are added.

Milestone completion does not require a theory note, retrospective, or written report. It does require working behavior and enough validation to trust the next layer. Measurements are required only when a milestone makes a performance claim.

The project owner chooses concrete dependencies and authors the implementation. Interface sketches in the architecture are constraints and vocabulary, not generated implementation.

## Phase A — Trustworthy foundations

### Milestone 0: repository and vertical skeleton

Establish the shared library, thin CLI, test target, and benchmark target using CMake and Conan. Define the composition root, typed configuration after parsing, `Result<T>`, structured errors, the Renderer front, and a minimal immutable Render Session lifecycle.

Use a programmatically constructed scene and a permanent diagnostic integrator to drive one complete request through Renderer, Film, snapshot, and an image-output adapter. Keep this deliberately small; do not introduce a backend registry, plugin ABI, GPU abstraction, or general job system.

Exit when the CLI can start and cancel a deterministic progressive session, retrieve a snapshot, and report a structured failure without global state.

### Milestone 1: numerical, spectral, and sampling foundations

Implement the scalar semantic math layer: points, vectors, normals, matrices, transforms, rays, ray intervals, and bounds. Fix coordinate handedness, transform conventions, normal handling, NaN/Inf diagnostics, and ray-origin policy before those choices spread.

Implement the spectral foundation following PBRT v4's general model: scene-side spectra, sampled wavelengths and their PDFs, Transport Spectra, sensor/color matching, and conversion to linear image channels. RGB remains an authored/display value.

Implement Sample Addresses and a deterministic independent sampler. A sample must be reproducible from pixel, sample index, dimension, and seed without mutable thread ordering.

Exit when analytic math and transform tests pass, spectral/color reference cases are credible, sample addresses replay exactly, and changing execution order cannot change requested sample values.

### Milestone 2: scene, camera, geometry, and film

Introduce the immutable Scene Model and stable IDs. Add a pinhole camera, sensor-aware Film, programmatic scene builder, shapes, primitives, and a brute-force aggregate. Start with spheres and triangles, object transforms, closest-hit, and any-hit queries.

Keep geometry binding separate: shapes own geometry behavior; primitives attach material, emission, transform, and later media; aggregates accelerate primitives.

Extend the diagnostic integrator with normals, depth, IDs, and simple visibility modes. Add OpenEXR as canonical output and PNG as a tone-mapped preview without putting file I/O into Film.

Exit when tiny scenes render deterministic diagnostic images, randomized intersection tests agree with analytic cases, transformed normals and bounds are correct, and Film snapshots remain safe during progressive accumulation.

## Phase B — Scalar physically based rendering

### Milestone 3: first spectral path-tracing slice

Create the PBRT-style CPU ladder: whole-render Integrator, image/tile orchestration, and a scalar camera-ray integrator whose narrow hook is `Li()`. Keep it single-threaded first so path behavior is easy to inspect.

Implement emitted radiance, Lambertian reflection, BSDF sampling with matching evaluation and PDF operations, spectral throughput, path depth, and deterministic termination limits. Use the brute-force aggregate and programmatic scenes.

Exit when emissive and diffuse scenes converge plausibly, fixed Sample Addresses reproduce complete paths, invalid values identify their originating pixel/sample, and BSDF sampling agrees with evaluation and PDF tests.

### Milestone 4: complete the Baseline Path Tracer

Turn the first slice into the permanent modern baseline:

- Light sampling for point, directional, area, and environment emitters as needed by validation scenes.
- Next-event estimation.
- Multiple importance sampling with correct delta-event handling.
- Russian roulette with survival compensation.
- Smooth dielectric and conductor Fresnel.
- GGX microfacet reflection and transmission, including visible-normal sampling.
- Thin-lens camera after the pinhole path is stable.

Keep light, shape, and BSDF sampling next to their PDF evaluation. Preserve a simpler path configuration where it helps isolate failures.

Exit when direct-light cases agree with analytic or high-sample references, NEE+MIS agrees with the simpler unbiased estimator while reducing variance on chosen scenes, furnace/energy tests behave correctly, and the baseline can explain any fixed path step by step.

### Milestone 5: assets and representative scenes

Add textures and a glTF 2.0 importer that converts assets into the Scene Model rather than leaking glTF types into rendering. Support the subset needed for triangles, hierarchy, transforms, cameras, textures, emissive input, and metallic-roughness material validation. Keep research-specific lights and settings in workbench configuration or programmatic scene construction.

Add instancing semantics before backend-specific layouts. Build a small permanent corpus: analytic micro-scenes, Cornell Box data, difficult glossy/lighting cases, and a few glTF sample assets. PBRT scene compatibility remains optional.

Exit when imported and programmatic versions of the same small scene agree, unsupported glTF features produce structured diagnostics, and canonical OpenEXR output contains enough metadata to reproduce the render configuration.

## Phase C — CPU acceleration and explicit SIMD

### Milestone 6: acceleration structures

Add acceleration incrementally while retaining brute force:

1. BVH2 with a simple median split.
2. Binned surface-area-heuristic construction.
3. Iterative closest-hit and any-hit traversal.
4. Two-level instancing.
5. Refit/rebuild experiments only when animated or changing scenes require them.

Prepared CPU scene data may now diverge from the Scene Model. Do not optimize node width, quantization, Morton construction, or spatial splits before categorized measurements justify an experiment.

Exit when brute-force and BVH hit records agree on randomized and adversarial rays, image results agree, and reports separate build time, traversal time, node visits, and memory.

### Milestone 7: deterministic multicore scalar rendering

Add a persistent CPU scheduler through explicit Renderer dependencies or Render Context, not a singleton. Parallelize progressive tiles or sample waves while keeping sampler semantics independent of worker order. Give workers isolated samplers or stateless sample access, scratch arenas, and local accumulation where needed.

Keep a one-thread configuration in the same scalar adapter. Avoid changing transport semantics to accommodate scheduling.

Exit when scalar output is bit-reproducible across supported thread counts under the defined accumulation order, cancellation and snapshots remain race-free, and scaling is reported separately for scene preparation and rendering.

### Milestone 8: narrow SIMD foundation

Implement only the SIMD primitives required by real kernels:

- Pack and mask types with a scalar reference adapter.
- ARM NEON as the first hardware adapter.
- Masked arithmetic, comparisons, selection, loads/stores, and horizontal operations as demanded.
- Batched ray/AABB and ray/triangle intersection.
- SIMD BVH traversal experiments.

Keep ISA code in target-specific translation units. Do not turn scalar `Vec3f` into a SIMD type and do not build a general-purpose SIMD library. Add AVX2 only when an x86 machine is available for development and measurement.

Exit when scalar and SIMD kernels agree on randomized and adversarial inputs, masked inactive lanes cannot affect results, and microbenchmarks report lane utilization as well as throughput.

### Milestone 9: SIMD path-integrator adapters

Add SIMD rendering beside the scalar Integrator:

1. Coherent ray packets for camera and shadow work.
2. Measure divergence as paths deepen.
3. Add stream/queue processing and compaction where packet utilization collapses.
4. Introduce private SoA or AoSoA Prepared Scene and path-state layouts only where measured.

The scalar and SIMD adapters share transport semantics and tests, not necessarily source layout or scheduling code. Configuration selects an adapter once at session startup.

Exit when SIMD and scalar render statistically equivalent images, categorized workloads explain where packets and streams win or lose, and results include rays/s, active-lane utilization, node visits, bytes/ray, and total frame time.

### CPU research-ready gate

The workbench is ready for broader research when it can:

- Render textured spectral scenes with diffuse, dielectric, conductor, and GGX materials using NEE+MIS.
- Reproduce a scalar path from its Sample Address.
- Compare brute force, scalar BVH, and SIMD traversal on coherent and incoherent rays.
- Produce raw OpenEXR, useful AOVs, and reproducible run metadata.
- Compare equal-sample and equal-time results against high-sample and independent references.

If these are not true, fix the foundation before adding frontier estimators.

## Phase D — GPU compute decision and experiments

### Milestone 10: select and establish the first GPU compute path

At this milestone, choose the API from actual available hardware and current ecosystem constraints. Prefer CUDA when regular NVIDIA access exists; evaluate OpenCL or another compute-only route explicitly if it does not. Do not let this deferred choice reshape the shared Scene Model.

Create a device-specific Prepared Scene with owned buffers, handwritten acceleration representation and traversal, explicit transfers, and RAII resource lifetime. Port the Baseline Path Tracer first as directly as the compute model permits, favoring a simple megakernel organization before building queues.

Exit when the GPU adapter renders the validation corpus without hardware ray-tracing APIs, reports transfer/preparation separately from rendering, and agrees statistically with the scalar baseline.

### Milestone 11: megakernel versus wavefront

Add a wavefront implementation beside the GPU megakernel:

- Camera-ray generation.
- Closest intersection.
- Material/BSDF evaluation.
- Shadow intersection.
- Continuing-path queues.
- Queue compaction and optional material sorting.

Run the same queue logic on CPU when practical for debugging, without expecting it to be the fastest CPU path. Preserve both GPU organizations when they remain useful experiments.

Exit when comparisons explain divergence, occupancy, register or live-state pressure, queue occupancy, compaction cost, memory traffic, and kernel-launch overhead on simple and heterogeneous scenes.

## Phase E — Physical breadth and research branches

### Milestone 12: participating media and spectral validation

Extend the trusted baseline with homogeneous absorption/scattering, phase functions, medium NEE and MIS, then heterogeneous null-collision tracking. Add wavelength-dependent Fresnel, absorption, dispersion, and explicit sensor/color validation cases. Motion time and transforms belong here if not required earlier.

Exit when homogeneous transmittance agrees with Beer-Lambert cases, null-collision estimators converge against homogeneous or dense references, and spectral results match selected colorimetric and dispersion cases.

### Research branch A: advanced transport

Implement independent adapters or modes in dependency order:

1. Bidirectional path tracing.
2. Metropolis light transport or a primary-sample-space variant.
3. Practical path guiding, then product guiding.
4. Adaptive sampling after trustworthy error estimates exist.

Each technique needs scenes that demonstrate both its advantage and its failure mode. Preserve the Baseline Path Tracer as the comparison oracle.

### Research branch B: reuse and interactive rendering

Progress through reservoir sampling exercises, ReSTIR DI, generalized resampled importance sampling, then ReSTIR GI or path resampling. Evaluate bias, temporal correlation, warm-up, disocclusion, and memory rather than image appearance alone.

Denoising follows raw-estimator validation. It consumes beauty and optional AOVs through an output/post-process adapter and never becomes evidence that the integrator is correct.

### Research branch C: differentiable and learned rendering

Begin with smooth-parameter derivatives in tiny scenes and an automatic-differentiation oracle. Add path replay backpropagation before visibility/geometry derivatives. Neural radiance caching comes later as a measured biased estimator with explicit training state and hardware costs.

These branches are not required in order. Choose one only when its research question becomes the active project goal.

## Ongoing validation and benchmark corpus

Maintain these permanent comparison layers as the workbench grows:

- Analytic unit and property cases for math, geometry, spectrum, sampling, and transport.
- Brute-force versus acceleration differential tests.
- Scalar versus SIMD/device differential or statistical tests.
- Fixed Sample Address path replay.
- High-sample reference images and convergence/error plots.
- Coherent primary/shadow, incoherent secondary, and feature-stress benchmark scenes.
- PBRT v4, Mitsuba 3, and Embree as external reference implementations, not hidden runtime dependencies.

Record source revision, scene revision, implementation, device, compiler and flags, resolution, sample budget, seed, sampler, integrator parameters, warm-up, time, memory, and whether output is raw or post-processed whenever publishing a performance result.

## Decision gates deliberately left open

- Exact spectrum packet size and wavelength sampling refinements after the first correct implementation.
- Concrete third-party dependencies.
- Stable external library/ABI policy.
- First GPU API and hardware.
- Whether packets, streams, or both remain first-class SIMD adapters after measurement.
- Advanced material interchange such as MaterialX/OpenPBR.
- First frontier research branch.

## Research references

The cited technical rationale and primary sources are collected in:

- [Modern path-tracing research roadmap](research/modern-path-tracing-roadmap.md)
- [PBRT v4 architecture reference](research/pbrt-v4-architecture-reference.md)
- [Modern C++ engineering standards](research/modern-cpp-engineering-standards.md)
