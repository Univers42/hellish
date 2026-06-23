# AI compute modes

The optional llama.cpp backend can run the local model three ways. Pick with
`make ai-up COMPUTE=<mode>`. Default is `cpu`.

| Mode     | Where the model runs        | Speed   | Needs              | Image          |
|----------|-----------------------------|---------|--------------------|----------------|
| `cpu`    | All on CPU                  | slowest | nothing            | Alpine, tiny   |
| `hybrid` | N layers GPU, rest CPU      | medium  | a Vulkan driver    | Debian, larger |
| `gpu`    | All layers on GPU           | fastest | Vulkan + enough VRAM | Debian, larger |

**Vulkan is vendor-agnostic** — `hybrid` and `gpu` use the same Vulkan image and
work on AMD, Intel, and NVIDIA GPUs reached through `/dev/dri`. No CUDA/ROCm
toolkit needed. NVIDIA users who want maximum performance can use the CUDA
image instead with `make ai-up COMPUTE=nvidia`.

## CPU

Universal, no drivers, works everywhere. Slowest. Fine for small models or
occasional use. Nothing to configure.

```sh
make ai-up                 # COMPUTE=cpu is the default
```

## Hybrid

Offload some of the model's layers to the GPU and keep the rest on the CPU. This
is the right mode when the model is **bigger than your VRAM**: it balances speed
against the VRAM you actually have.

```sh
make ai-up COMPUTE=hybrid NGL=24
```

`NGL` is `--n-gpu-layers` — how many layers go to the GPU. **How to pick N:**
start low (the default is 20), watch VRAM usage, and raise `NGL` until the GPU
VRAM is nearly full. More layers on the GPU = faster, until you run out of VRAM.

## GPU

All layers on the GPU. Fastest, but needs enough VRAM for the **whole** model and
a working Vulkan driver (the container reaches it via `/dev/dri`).

```sh
make ai-up COMPUTE=gpu     # NGL defaults to 999 = offload everything
```

## Verifying the GPU is used

Watch the server logs after `make ai-up`:

```sh
make ai-logs
```

llama-server prints the offload at startup, e.g. `offloaded 24/33 layers to GPU`
(hybrid) or `offloaded 33/33 layers to GPU` (full GPU). If it shows `0` layers
offloaded, the Vulkan driver isn't visible — check that `/dev/dri` exists on the
host and that Mesa (AMD/Intel) or the NVIDIA driver is installed.

Tear everything down with `make ai-down` (it stops all modes).
