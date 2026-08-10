# Modern Path-Tracing Research Roadmap

_Literature checked through 8 August 2026. This note orders learning and experiments; it does not prescribe product architecture._

## Executive recommendation

Build one small, trustworthy estimator before building a fast renderer. The enduring baseline is still Kajiya's rendering equation solved with Monte Carlo path tracing, strengthened by next-event estimation (NEE), multiple importance sampling (MIS), physically meaningful BSDFs, and Russian roulette. Kajiya introduced the rendering equation and path-tracing solution; Veach's thesis remains the primary reference for path-space transport, MIS, bidirectional methods, and Metropolis light transport ([Kajiya 1986](https://doi.org/10.1145/15886.15902), [Veach 1997](https://graphics.stanford.edu/papers/veach_thesis/)).

The recommended progression is:

1. a deterministic scalar surface path tracer as a correctness oracle;
2. a measured BVH and CPU SIMD study;
3. a GPU baseline followed by a wavefront/compaction study;
4. spectral and volumetric transport;
5. advanced unbiased sampling and integrators;
6. low-sample-count reuse and denoising experiments;
7. differentiable and neural experiments.

Do not start with ReSTIR, differentiable rendering, denoising, or a large material standard. Each depends on sampling densities, path throughput, visibility, and reproducibility that are much easier to verify in the baseline.

## What is foundational and what is experimental

| Class | Implement | Why it belongs here |
|---|---|---|
| **Must implement** | Rays, transforms, robust intersections, scalar vectors/matrices, cameras, film, deterministic sampler dimensions | These make every later CPU/GPU comparison meaningful. PBRT's system overview is a useful independent reference for the complete dataflow ([PBRT v4 overview](https://www.pbr-book.org/4ed/Introduction/pbrt_System_Overview)). |
| **Must implement** | Unidirectional path tracing, emission, NEE, MIS, Russian roulette | This is the minimum modern physically based integrator, not merely a “weekend” tracer ([Veach 1997](https://graphics.stanford.edu/papers/veach_thesis/)). |
| **Must implement** | Diffuse, smooth dielectric/conductor, GGX microfacet reflection/transmission, matching `evaluate`/`sample`/PDF logic | Correct importance sampling and delta-event bookkeeping are prerequisites for MIS. Visible-normal GGX sampling is the modern reference direction ([Heitz research and papers](https://eheitzresearch.wordpress.com/research/)). |
| **Must implement** | BVH2, binned surface-area-heuristic construction, iterative traversal, instancing | It provides a transparent acceleration baseline. PBRT documents a complete SAH BVH, while Embree exposes SAH, spatial-split, and Morton builders as high-performance comparison points ([PBRT BVH](https://pbr-book.org/4ed/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies), [Embree](https://github.com/RenderKit/embree)). |
| **Must measure** | Scalar versus SIMD kernels; megakernel versus wavefront GPU execution; software versus hardware traversal where available | None is universally fastest. Workload coherence, material complexity, queue overhead, register pressure, and hardware all change the result ([Laine, Karras, and Aila 2013](https://research.nvidia.com/publication/2013-07_megakernels-considered-harmful-wavefront-path-tracing-gpus), [PBRT GPU chapter](https://pbr-book.org/4ed/Wavefront_Rendering_on_GPUs)). |
| **Advanced experiment** | Spectral/volumetric transport, BDPT/MLT, path guiding, reservoirs, differentiation, neural caches, denoising | These are important research branches, but each should be compared with a converged and inspectable baseline. |

## Staged implementation and research milestones

### Stage 0 — Numerical and experimental ground rules

Implement scalar `float` math yourself, but make numerical behavior explicit: coordinate conventions, transform handedness, ray interval semantics, normal transformation, NaN/Inf handling, and epsilon/origin-offset policy. Test vectors, matrices, transforms, bounds, ray/triangle, and ray/AABB independently. Retain a brute-force intersector for tiny scenes so that BVH results can be checked hit-for-hit.

Sampling must be a reproducible input to an experiment: a sample is addressed by pixel, sample index, dimension, and seed. Start with an independent hash/PRNG sampler, then add stratified and Sobol-family sequences. PBRT expects identical sample coordinates for the same pixel and sample index across runs and explains why low-discrepancy sequences require disciplined dimension use ([PBRT sampling interface](https://pbr-book.org/4ed/Sampling_and_Reconstruction/Sampling_Interface), [sampling and integration](https://pbr-book.org/4ed/Sampling_and_Reconstruction/Sampling_and_Integration)). Progressive multi-jittered sequences are a valuable later comparison because every prefix is well distributed, which suits progressive and adaptive rendering ([Christensen, Kensler, and Kilpatrick 2018](https://graphics.pixar.com/library/ProgressiveMultiJitteredSampling/)).

**Exit evidence:** analytic intersection tests pass; brute-force images are repeatable; changing thread count does not change generated samples; invalid floating-point values are reported with pixel/sample provenance.

### Stage 1 — The scalar reference path tracer

Implement surface transport in deliberately small increments:

1. pinhole and thin-lens cameras, emissive surfaces, and Lambertian reflection;
2. cosine/BSDF importance sampling and throughput updates with their actual PDFs;
3. NEE with sampled point, directional, area, and environment lights;
4. MIS between light and BSDF sampling, with correct handling of delta lights and delta BSDF events;
5. Russian roulette with survival-probability compensation;
6. smooth dielectric and conductor Fresnel, then GGX reflection/transmission and visible-normal sampling;
7. motion time and transforms only after the static estimator is stable.

The critical learning artifact is not just an image: for every scattering model, numerically test PDF normalization, sampled-versus-evaluated consistency, finite values at grazing angles, approximate energy conservation, and reciprocity where the model promises it. Veach explains why transport measures and delta distributions matter to MIS rather than being implementation details ([Veach 1997, chapters 3 and 9](https://graphics.stanford.edu/papers/veach_thesis/)).

**Exit evidence:** diffuse furnace tests behave as expected; direct lighting agrees with an analytic or high-sample reference; NEE+MIS is unbiased relative to the BSDF-only estimator and has lower variance on chosen scenes; fixed-seed path logs explain individual pixels.

### Stage 2 — BVHs and CPU SIMD

Use a progression that exposes the algorithm rather than jumping directly to a production library:

1. BVH2 with median split;
2. binned SAH construction;
3. iterative closest-hit and any-hit traversal;
4. two-level instancing and refit/rebuild experiments;
5. wide BVHs, compressed/quantized bounds, spatial splits, or Morton/LBVH only as measured variants.

For SIMD, first vectorize isolated kernels: vector arithmetic, ray/AABB slabs, triangle intersection, and small BSDF batches. Then compare two strategies:

- **packets:** several rays traverse together, efficient when rays remain coherent;
- **streams/queues:** rays are compacted or sorted and processed in structure-of-arrays/AoSoA batches, usually more resilient after paths diverge.

Measure active-lane utilization, rays/s, node visits, bytes/ray, build time, and memory—not only full-frame time. Embree is a valuable external baseline because it explicitly targets coherent and incoherent rays, supports ray packets and runtime-selected SSE/AVX/AVX2/AVX-512 kernels, and supplies optimized BVH builders ([Embree 4](https://github.com/RenderKit/embree)). For handwritten backends, use the vendor specifications as the source of truth: the current Arm Neon intrinsic reference is ACLE 2026Q1, while Intel maintains the x86 intrinsic guide ([Arm Neon intrinsics](https://arm-software.github.io/acle/neon_intrinsics/advsimd.html), [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)). ISPC is worth benchmarking as a compiler-generated SPMD reference, not as a replacement for learning explicit SIMD ([ISPC](https://ispc.github.io/)).

**Exit evidence:** scalar and SIMD hit records agree on randomized rays and adversarial boundary cases; performance reports separate construction, primary, shadow, and incoherent secondary-ray workloads; speedups include lane-utilization data.

### Stage 3 — GPU execution experiments

First port the scalar estimator as directly as the GPU API allows, then introduce explicit wavefront queues. A wavefront path tracer splits generation, intersection, material evaluation, shadow testing, and accumulation into stages, compacting live work between them. The original wavefront study identifies control-flow divergence and register pressure as the motivation; PBRT v4 provides a modern, full renderer implementation and discussion ([Laine et al. 2013](https://research.nvidia.com/publication/2013-07_megakernels-considered-harmful-wavefront-path-tracing-gpus), [PBRT wavefront implementation](https://pbr-book.org/4ed/Wavefront_Rendering_on_GPUs/Path_Tracer_Implementation)).

Run the comparison rather than assuming the answer: simple scenes can favor a megakernel, while heterogeneous materials and long-lived state often favor wavefront execution. Record queue occupancy, compaction cost, divergence, register/live-state size, memory traffic, and kernel launch time. Test sorting by material or path state only after plain compaction is understood.

Hardware ray traversal should be an experimental variable. Vulkan defines both a ray-tracing pipeline and ray queries over opaque acceleration structures; Metal exposes acceleration structures plus intersectors/intersection queries ([Vulkan ray tracing specification](https://docs.vulkan.org/spec/latest/chapters/raytracing.html), [Apple Metal ray tracing](https://developer.apple.com/documentation/metal/ray-tracing-with-acceleration-structures)). As of 2026, Vulkan's ratified `VK_EXT_ray_tracing_invocation_reorder` also exposes hit objects and invocation reordering to improve coherence; treat it as an advanced comparison with software queues, not a baseline dependency ([Khronos extension](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_ray_tracing_invocation_reorder.html)).

**Exit evidence:** CPU scalar, CPU SIMD, and GPU backends agree within defined floating-point/image tolerances; a report explains when megakernel, wavefront, and hardware traversal win on the same scenes and sample streams.

### Stage 4 — Spectral and volumetric transport

Add spectral rendering after RGB transport is trustworthy. A good educational sequence is monochromatic tests, sampled wavelengths, camera/sensor response and XYZ/RGB conversion, RGB-to-spectrum reconstruction, then wavelength-dependent Fresnel, absorption, and dispersion. Hero-wavelength sampling propagates a small fixed set of wavelengths while using one wavelength to choose directions; PBRT v4 offers a current alternative reference that samples four wavelengths per camera path and terminates secondary wavelengths when dispersion separates paths ([Wilkie et al. 2014](https://cgg.mff.cuni.cz/publications/hero-wavelength-spectral-sampling/), [PBRT spectral representation](https://www.pbr-book.org/4ed/Radiometry%2C_Spectra%2C_and_Color/Representing_Spectral_Distributions), [PBRT color](https://pbr-book.org/4ed/Radiometry%2C_Spectra%2C_and_Color/Color)).

For volumes, begin with homogeneous absorption/scattering and phase functions, then heterogeneous delta/null-collision tracking, medium NEE, and MIS. The null-scattering path-integral formulation unifies spectral tracking and NEE/MIS for heterogeneous media and is the modern theoretical reference used by PBRT v4's volumetric integrator ([Miller, Georgiev, and Jarosz 2019](https://cs.dartmouth.edu/~wjarosz/publications/miller19null.html), [PBRT v4 user guide](https://pbrt.org/users-guide-v4)).

**Exit evidence:** spectral results match known colorimetric cases; dispersion splits wavelengths without losing estimator weight; homogeneous-medium transmittance agrees with Beer–Lambert; null-collision estimators converge against homogeneous or dense references.

### Stage 5 — Advanced unbiased transport and adaptive sampling

Implement these as separate experiments, in roughly this order:

- **Bidirectional path tracing (BDPT):** teaches path-space measures, camera/light subpaths, connection strategies, and MIS. It pays off on difficult indirect lighting and some caustics but substantially increases bookkeeping ([Veach 1997](https://graphics.stanford.edu/papers/veach_thesis/)).
- **Metropolis light transport / primary-sample-space variants:** useful for rare-path scenes, with correlation and startup behavior that demand different evaluation than independent path tracing ([Veach's MLT project](https://graphics.stanford.edu/papers/metro/)).
- **Path guiding:** learn an incident-radiance distribution online and mix it safely with BSDF sampling. Practical Path Guiding uses a spatial-directional tree; product guiding additionally targets incident radiance times the BSDF ([Müller, Gross, and Novák 2017](https://www.research-collection.ethz.ch/handle/20.500.11850/190776), [Diolatzis et al. 2020](https://diglib.eg.org/items/8846bd4e-448e-4732-99c3-809f53e04546)).
- **Adaptive sampling:** only after per-pixel variance/error estimates and progressive sample sequences are trustworthy. Evaluate error per ray and per second, and verify that stopping rules do not bias comparisons.

**Exit evidence:** each method has scenes that reveal both its advantage and failure mode; all unbiased/consistent methods show convergence plots against the same high-sample references.

### Stage 6 — Reservoir reuse and denoising

Reservoir methods are a research ladder, not one feature:

1. reservoir sampling and resampled importance sampling in small standalone tests;
2. ReSTIR DI for many dynamic lights;
3. generalized resampled importance sampling (GRIS), including confidence/normalization and correlation assumptions;
4. ReSTIR GI or path resampling;
5. recent path-guiding, full-path, and robustness work.

The original ReSTIR paper derives direct-lighting estimators with spatial and temporal reuse; GRIS generalizes the theory, and ReSTIR GI extends reuse to indirect paths ([Bitterli et al. 2020](https://research.nvidia.com/labs/rtr/publication/bitterli2020spatiotemporal/), [Lin et al. 2022](https://research.nvidia.com/labs/rtr/publication/lin2022generalized/), [Ouyang et al. 2021](https://research.nvidia.com/publication/2021-06_restir-gi-path-resampling-real-time-path-tracing)). As of August 2026, active directions include ReSTIR-derived path guiding, reservoir splatting/multiple layers for temporal disocclusion, compatibility-guided neighbor choice, and gradient-domain path reuse ([ReSTIR-PG 2025](https://research.nvidia.com/labs/rtr/publication/zeng2025restirpg/), [reservoir splatting 2025](https://research.nvidia.com/labs/rtr/publication/liu2025splatting/), [multi-layer splatting 2026](https://research.nvidia.com/labs/rtr/publication/hong2026multilayer/), [compatibility-guided reuse 2026](https://research.nvidia.com/labs/rtr/publication/junkins2026compatibility/), [gradient-domain ReSTIR 2026](https://research.nvidia.com/labs/rtr/publication/wang2026gradient/)). These are frontier experiments: reproduce DI/GRIS first, and report bias, temporal correlation, disocclusion behavior, memory, and warm-up—not only attractive frames.

Denoising belongs after raw-estimator validation. Preserve noisy beauty and auxiliary buffers, and never use a denoised image as proof of transport correctness. Open Image Denoise accepts beauty with optional albedo/normal guides; OptiX additionally documents temporal and AOV modes ([Open Image Denoise](https://www.openimagedenoise.org/documentation.html), [OptiX 9 programming guide](https://raytracing-docs.nvidia.com/optix9/guide/optix_guide.250130.LTR.pdf)). Denoising is useful for interactive presentation and for studying the renderer/denoiser contract, but it is not an integrator milestone.

### Stage 7 — Differentiable and neural experiments

Differentiable rendering should begin with derivatives of smooth parameters and a tiny scene. Naively recording an automatic-differentiation graph is a useful oracle; path replay backpropagation (PRB) is the serious Monte Carlo milestone because it reconstructs local quantities during a replay, giving linear time and constant memory in path length for its supported cases ([Vicini, Speierer, and Jakob 2021](https://dvicini.github.io/path-replay-backpropagation/)). Geometry and visibility derivatives require additional estimators/reparameterizations; do not infer their correctness from material-only tests. Mitsuba 3 is the primary executable reference because it exposes retargetable scalar/vectorized/GPU variants and dedicated differentiable integrators ([Mitsuba 3](https://mitsuba-renderer.org/), [Mitsuba API](https://mitsuba.readthedocs.io/en/v3.6.3/src/api_reference.html)).

Neural radiance caching is a later GPU experiment: it trains a cache online while rendering dynamic scenes and trades a small amount of bias for low-noise multi-bounce estimates ([Müller et al. 2021](https://research.nvidia.com/publication/2021-06_real-time-neural-radiance-caching-path-tracing)). Treat neural methods as learned estimators with training state, generalization limits, and hardware-specific costs—not as replacements for the reference path tracer.

## Validation corpus and standards

Use several layers of evidence:

| Layer | Recommended evidence |
|---|---|
| Unit/property | Analytic intersections; random scalar/SIMD differential tests; transform round trips; BSDF/PDF normalization, energy, reciprocity, and finite-value tests; sampler repeatability and distribution tests. |
| Path-level | Fixed-seed path transcripts containing vertices, event types, throughput factors, PDFs, MIS weights, visibility, and termination reason. Compare transports event-by-event before comparing pixels. |
| Image-level | High-sample references; convergence versus samples and wall time; absolute/relative error and outlier maps; separate static and animated sequences for temporal methods. |
| External implementations | PBRT v4 for spectral/offline transport, Mitsuba 3 for research and differentiation, and Embree for CPU traversal. These are comparison oracles, not runtime dependencies ([PBRT](https://pbrt.org/), [Mitsuba 3](https://mitsuba-renderer.org/), [Embree](https://github.com/RenderKit/embree)). |
| Scenes | Cornell's measured box data for a small reference, Veach scenes for difficult transport, PBRT v4 scenes for feature breadth, and Khronos sample assets for importer/material coverage ([Cornell Box](https://bowers.cornell.edu/cornell-box), [PBRT resources](https://www.pbrt.org/resources), [Khronos glTF Sample Assets](https://github.com/KhronosGroup/glTF-Sample-Assets)). |

For interchange, glTF 2.0 is a practical first validation target for geometry, transforms, cameras, textures, and its metallic-roughness PBR model; it is an asset-delivery standard, not a full research scene language ([glTF 2.0 specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html)). MaterialX 1.39 and OpenPBR are later references for richer material graphs and standardized surface semantics ([MaterialX specification](https://materialx.org/Specification.html), [OpenPBR specification](https://github.com/AcademySoftwareFoundation/OpenPBR)). OpenEXR should carry linear HDR beauty, AOVs, and experiment metadata because it supports floating-point pixels, arbitrary channels, and arbitrary attributes ([OpenEXR technical introduction](https://openexr.com/en/latest/TechnicalIntroduction.html)).

## Benchmark discipline

Every result should capture source revision, scene and asset revision, backend/device, compiler and flags, thread/workgroup configuration, resolution, spp/ray budget, seed and sampler, integrator settings, warm-up, build time, render time, peak memory, and whether the reported image is raw or denoised. Report both equal-sample and equal-time results: SIMD, wavefront compaction, hardware traversal, reuse, and neural methods all change per-sample cost.

Keep at least three benchmark classes: coherent primary/visibility rays, incoherent multi-bounce paths, and feature stress scenes (many lights, glossy indirect illumination, caustics, volumes, motion/disocclusion). A speedup without image error, ray count, and workload description is not interpretable.

## Suggested stopping point before frontier work

The workbench is ready for modern research experiments when it can:

- render a static textured scene with diffuse, dielectric, conductor, and GGX materials using NEE+MIS;
- produce deterministic scalar results and statistically equivalent SIMD/GPU results;
- compare brute force, own BVH, hardware traversal, and Embree on categorized ray workloads;
- emit raw linear OpenEXR plus useful AOVs and complete run metadata;
- show convergence plots and inspect one sample path end-to-end;
- reproduce at least one spectral and one participating-media validation case.

At that point, path guiding and ReSTIR DI are the best first “modern research” projects: both build directly on correct PDFs and MIS, visibly improve hard scenes, and force careful measurement. PRB and neural caching should follow only when the corresponding differentiation or learned-estimation question becomes the explicit focus.
