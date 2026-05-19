# syntax=docker/dockerfile:1.7

FROM alpine:edge

ARG EDGE_MAIN=https://dl-cdn.alpinelinux.org/alpine/edge/main
ARG EDGE_COMMUNITY=https://dl-cdn.alpinelinux.org/alpine/edge/community
ARG EDGE_TESTING=https://dl-cdn.alpinelinux.org/alpine/edge/testing
ARG WFX_PYTHON_VERSION=3.14.3
ARG UV_VERSION=0.11.15
ARG TARGETARCH

RUN printf '%s\n' "$EDGE_MAIN" "$EDGE_COMMUNITY" "$EDGE_TESTING" > /etc/apk/repositories

RUN --mount=type=cache,target=/var/cache/apk \
    apk update --cache-dir /var/cache/apk && \
    apk_arch=$(apk --print-arch) && \
    mkdir -p /tmp/wfx-python3-dummy && \
    apk mkpkg \
      --files /tmp/wfx-python3-dummy \
      --info name:python3 \
      --info version:3.14.3-r0 \
      --info arch:"$apk_arch" \
      --info description:"dummy python3 dependency provider; real Python is installed by uv" \
      --info license:MIT \
      --output /tmp/wfx-python3-dummy.apk && \
    apk add --cache-dir /var/cache/apk --allow-untrusted --no-progress \
      /tmp/wfx-python3-dummy.apk \
      autoconf \
      automake \
      bash \
      bzip2 \
      bison \
      ca-certificates \
      cargo \
      cbindgen \
      clang22 \
      clang22-dev \
      clang22-libclang \
      cmake \
      compiler-rt \
      coreutils \
      curl \
      file \
      findutils \
      flex \
      bsd-compat-headers \
      fontconfig-dev \
      freetype-dev \
      gperf \
      git \
      libc++-dev \
      libtool \
      libxkbcommon-dev \
      linux-headers \
      lld22 \
      llvm22 \
      m4 \
      make \
      mold \
      nasm \
      nodejs-current \
      patch \
      pax-utils \
      perl \
      pkgconf \
      sccache \
      tar \
      unzip \
      wayland-dev \
      wayland-protocols \
      xz \
      yasm \
      zip \
      zlib-dev \
      zstd \
      !pyc \
      !python3-pyc \
      !python3-pycache-pyc0 \
      !xcb-proto-pyc && \
    rm -rf /tmp/wfx-python3-dummy /tmp/wfx-python3-dummy.apk

RUN case "$TARGETARCH" in \
      amd64) uv_target=x86_64-unknown-linux-musl ;; \
      arm64) uv_target=aarch64-unknown-linux-musl ;; \
      *) printf 'unsupported uv TARGETARCH: %s\n' "$TARGETARCH" >&2; exit 1 ;; \
    esac && \
    uv_archive="uv-$uv_target.tar.gz" && \
    uv_base="https://github.com/astral-sh/uv/releases/download/$UV_VERSION" && \
    curl -fL "$uv_base/$uv_archive" -o "/tmp/$uv_archive" && \
    curl -fL "$uv_base/$uv_archive.sha256" -o "/tmp/$uv_archive.sha256" && \
    cd /tmp && sha256sum -c "$uv_archive.sha256" && \
    tar -xzf "$uv_archive" && \
    install -m 0755 "/tmp/uv-$uv_target/uv" /usr/local/bin/uv && \
    install -m 0755 "/tmp/uv-$uv_target/uvx" /usr/local/bin/uvx && \
    rm -rf "/tmp/uv-$uv_target" "/tmp/$uv_archive" "/tmp/$uv_archive.sha256" && \
    uv python install "$WFX_PYTHON_VERSION" --install-dir /opt/wfx/python --no-bin --compile-bytecode && \
    python_bin=$(find /opt/wfx/python -path '*/bin/python3' \( -type f -o -type l \) | sort | head -n 1) && \
    [ -n "$python_bin" ] && \
    ln -sf "$python_bin" /usr/local/bin/python3 && \
    ln -sf "$python_bin" /usr/local/bin/python && \
    ln -sf "$python_bin" "/usr/local/bin/python${WFX_PYTHON_VERSION%.*}"

COPY pyproject.toml /opt/wfx/python-tools/pyproject.toml

RUN uv venv --python /usr/local/bin/python3 /opt/wfx/python-tools/venv && \
    uv pip install --link-mode=copy \
      --python /opt/wfx/python-tools/venv/bin/python \
      --group /opt/wfx/python-tools/pyproject.toml:wfx-tools

ENV PATH=/opt/wfx/python-tools/venv/bin:/usr/lib/llvm22/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
ENV CC=clang-22
ENV CXX=clang++-22
ENV HOST_CC=clang-22
ENV HOST_CXX=clang++-22
ENV LD=mold
ENV AR=/usr/lib/llvm22/bin/llvm-ar
ENV RANLIB=/usr/lib/llvm22/bin/llvm-ranlib
ENV NM=/usr/lib/llvm22/bin/llvm-nm
ENV STRIP=/usr/lib/llvm22/bin/llvm-strip
ENV READELF=/usr/lib/llvm22/bin/llvm-readelf
ENV OBJCOPY=/usr/lib/llvm22/bin/llvm-objcopy
ENV LLVM_CONFIG=llvm-config-22
ENV RUSTC_WRAPPER=sccache
ENV MOZBUILD_STATE_PATH=/cache/mozbuild
ENV SCCACHE_DIR=/cache/sccache
ENV CARGO_HOME=/cache/cargo
ENV WFX_SYSROOT=/opt/wfx/sysroot
ENV WFX_CACHE=/cache
ENV UV_PYTHON=/usr/local/bin/python3

RUN cat > /usr/local/bin/wfx-toolchain-report <<'EOF'
#!/bin/sh
set -eu
printf 'alpine='
cat /etc/alpine-release
printf 'arch='
apk --print-arch
clang-22 --version | sed -n '1p'
clang++-22 --version | sed -n '1p'
ld.lld --version | sed -n '1p'
mold --version
ninja --version
meson --version
rustc --version
cargo --version
python3 --version
uv --version
cbindgen --version
sccache --version
EOF

RUN chmod +x /usr/local/bin/wfx-toolchain-report && wfx-toolchain-report

WORKDIR /work
