# Toolchain container for the Silent Bomber (SLUS_009.02) decompilation.
# Adapted from the Parasite Eve 2 decomp project's Dockerfile.
#
# Build:  docker build -t silent-bomber-decomp decomp
# Use:    docker run --rm -v "$PWD":/app -w /app/decomp silent-bomber-decomp make
#         docker run --rm -v "$PWD":/app -w /app/decomp silent-bomber-decomp make split
#
# linux/amd64 is pinned because the vendored gcc 2.x cc1 binaries
# (tools/compilers/linux) are statically linked 32-bit x86 executables; on
# arm64 hosts Docker runs the container through Rosetta/qemu automatically.

FROM --platform=linux/amd64 ubuntu:25.10

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    make \
    python3 \
    python3-pip \
    python3-venv \
    binutils-mipsel-linux-gnu \
    ca-certificates \
    git \
    && rm -rf /var/lib/apt/lists/*

# splat/spimdisasm/rabbitizer versions pinned to the ones used for the
# current split (see configs/USA/main.yaml header comment).
RUN python3 -m venv /venv \
    && /venv/bin/pip install --no-cache-dir \
        splat64==0.50.0 \
        spimdisasm==1.42.4 \
        rabbitizer==1.16.2 \
        crunch64==0.6.2 \
        intervaltree==3.1.0 \
        n64img==0.3.3 \
        pygfxd==1.0.5 \
        pypng==0.20220715.0

ENV PATH="/venv/bin:${PATH}"

WORKDIR /app
