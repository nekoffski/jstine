# Use sampled spectral transport from the baseline

The Baseline Path Tracer will carry sampled wavelengths and Transport Spectra from its first complete implementation, following PBRT v4's general approach. This accepts additional early work in color science, wavelength sampling, and film conversion so that RGB assumptions do not become embedded across materials, lights, integrators, and optimized backends; RGB remains an input and display representation rather than the representation of light transport.
