# Rendering Workbench

A long-lived sandbox for implementing, comparing, and benchmarking physically based rendering techniques.

## Language

**Rendering Workbench**:
The complete project: a reusable rendering core, tools for reproducible experiments, and the frontends used to operate them.
_Avoid_: App, demo, ray tracer

**Scene Model**:
The device-independent description of a scene's geometry, materials, lights, cameras, and rendering semantics.
_Avoid_: Backend scene, GPU scene

**Render Backend**:
A named execution strategy that an Integrator adapter uses to prepare a Scene Model and perform rendering.
_Avoid_: Renderer, device abstraction, execution mode

**Renderer**:
The front door that coordinates rendering a Scene Model into results using one configured Integrator adapter.
_Avoid_: Integrator, backend, tracer

**Integrator**:
A light-transport algorithm that estimates the image-forming contribution of paths through a Scene Model.
_Avoid_: Renderer, execution backend, shader

**Transport Spectrum**:
A sampled representation of wavelength-dependent radiometric quantities carried and evaluated along a light path.
_Avoid_: RGB, color, Vec3

**Sample Address**:
The stable identity of a random sample, formed from its pixel, sample index, dimension, and experiment seed independently of execution order.
_Avoid_: RNG state, thread seed

**Render Session**:
One active, progressive rendering operation together with its control, progress, and results.
_Avoid_: Render job, render process

**Baseline Path Tracer**:
The first complete progressive unidirectional path tracer, used as the correctness and comparison baseline for later rendering techniques and backends.
_Avoid_: V1 renderer, throwaway tracer, Whitted renderer
