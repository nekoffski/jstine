# Address samples independently of scheduling

Random samples will be identified by pixel, sample index, dimension, and experiment seed rather than by mutable thread-local progression. This makes scalar results reproducible across thread counts, lets failing paths be replayed directly, and gives SIMD and future GPU adapters the same sampling semantics; optimized implementations may differ in floating-point evaluation order and are therefore required to be statistically or numerically equivalent rather than bit-identical.
