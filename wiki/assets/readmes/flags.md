# FAST OPTIMIZATION FLAGS :
- `-03`: Maximum optimization level
- `-march=native -mtune=native`: optimize for our specific CPU architecture
- `-flto`: Linke-Time Optimization for cross-file optimizations
- `-ffast-math`: Aggressive floating-point optimizations
- `-funroll-loops`: unroll loops for better performance
- `-finline-functions`: inline function aggresively
- `-fomit-frame-pointer`: remove frame pointer for speed
- `-fno-stack-protector`: Remove stack protection overhead
- `DNDEBUG`: Disable debug assertion
- `-fdata-sections -ffunction-sections`: enable dead code elimination
- `-pipe`: use pipes instead of temp files during compilation
- `-ftree-vectorize`: auto-vectorization
- `-fgsce-cp-clone` interprocedural constant propagation
- `-fpredictive-commoning`: Predictive commoning optimization
- `-fipa-cp-clone`: interprocedural constant propogation
- `-fvect-cost-model=cheap`: agressive vectorization


## Linker optimization

- flto: link-time optimization
- `Wl, --gc-sections`: remove unused sections
- `Wl,  O1` linker optimization
- `Wl, --as-needed`: Only link needed libraries
- `strip`: remove debugging symbols in optimized builds