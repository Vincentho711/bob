FROM ubuntu:24.04

# Pinned tool versions — matches the Ubuntu 24.04 LTS development environment.
# To upgrade a tool: update the pin here, open a PR, merge, then run
# scripts/update-env.sh <new-tag> to deploy the new .sif to shared storage.
#
#   verilator : 5.020  (apt package 5.020-1)
#   gcc / g++ : 13.3.0 (Ubuntu 24.04 default via gcc metapackage)
#   ar        : 2.42   (binutils, Ubuntu 24.04 default)
#   python    : 3.12   (Ubuntu 24.04 default)
#   uv        : 0.9.2  (pinned below via official image)

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    verilator=5.020-1 \
    gcc \
    g++ \
    binutils \
    python3.12 \
    python3.12-venv \
    curl \
    git \
    && rm -rf /var/lib/apt/lists/*

# uv: pinned version copied directly from the official uv image
COPY --from=ghcr.io/astral-sh/uv:0.9.2 /uv /usr/local/bin/uv

# Validate tool installation at build time
RUN verilator --version && gcc --version && g++ --version && ar --version && \
    python3.12 --version && uv --version

WORKDIR /workspace
