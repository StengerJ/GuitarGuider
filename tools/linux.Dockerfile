# Linux build/test environment (also used for headless UI screenshots via Xvfb).
#   docker build -f tools/linux.Dockerfile -t guitarguider-build tools
#   docker run --rm -v "$PWD":/src guitarguider-build sh -c "cmake -S /src -B /build -G Ninja && cmake --build /build && /build/GuitarGuiderTests"
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build pkg-config git ca-certificates \
        libfreetype-dev libfontconfig1-dev libasound2-dev \
        libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libxcomposite-dev libxrender-dev \
        libgl1-mesa-dev xvfb xauth fonts-dejavu-core \
    && rm -rf /var/lib/apt/lists/*
