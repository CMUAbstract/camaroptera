**Toolchain Stuff:**

- [] Compatibility with mspgcc-{6,7,8,9} 
- [] Unify linker scripts to define the same sections across gcc versions?

**Peripheral Stuff:**
- [] Camera RAMBuffer needs to be mapped to SRAM
- [] Camera RAMBuffer -- use entire SRAM and DMA all at once?
- [] Camera -- Remove magic delay numbers where possible

- [] LoRa -- create `lite` versions; statically define structs/functions based on values known at compile time
- [] JPEG -- create `lite` versions; statically define structs/functions based on values known at compile time

**Compute Optimizations:**
- [] Use SRAM for temp values instead of FRAM?
- [] Hunt down stray multiplications and rotate ops -- super expensive
- [] f_mul has a multiply AND a rotate!!
- [] f_mul casts both inputs to 2x the fixed-point size, verify that this is indeed necessary

**Memory Stuff:**
- [] Define asymmetric double-buffers for DNN using largest and second-largest activation size requirements

