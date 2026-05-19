# syntax=docker/dockerfile:1.7

FROM alpine:edge

ARG EDGE_MAIN=https://dl-cdn.alpinelinux.org/alpine/edge/main
ARG EDGE_COMMUNITY=https://dl-cdn.alpinelinux.org/alpine/edge/community
ARG EDGE_TESTING=https://dl-cdn.alpinelinux.org/alpine/edge/testing

RUN printf '%s\n' "$EDGE_MAIN" "$EDGE_COMMUNITY" "$EDGE_TESTING" > /etc/apk/repositories

RUN --mount=type=cache,target=/var/cache/apk \
    apk update --cache-dir /var/cache/apk && \
    apk add --cache-dir /var/cache/apk --no-progress \
      autoconf \
      automake \
      alsa-lib-dev \
      bash \
      bzip2 \
      bison \
      ca-certificates \
      cargo \
      cbindgen \
      clang21 \
      clang21-libclang \
      clang22 \
      clang22-dev \
      cmake \
      compiler-rt \
      coreutils \
      curl \
      file \
      findutils \
      flex \
      gettext-dev \
      bsd-compat-headers \
      cairo-dev \
      fontconfig-dev \
      freetype-dev \
      fribidi-dev \
      gdk-pixbuf-dev \
      glib-dev \
      gtk+3.0-dev \
      gperf \
      git \
      harfbuzz-dev \
      libc++-dev \
      libepoxy-dev \
      libffi-dev \
      libjpeg-turbo-dev \
      libpng-dev \
      libtool \
      libxkbcommon-dev \
      linux-headers \
      lld22 \
      llvm22 \
      llvm22-dev \
      m4 \
      make \
      mesa-dev \
      meson \
      mold \
      nasm \
      ninja-build \
      nodejs-current \
      pango-dev \
      patch \
      pax-utils \
      pcre2-dev \
      pixman-dev \
      perl \
      pkgconf \
      py3-zstandard \
      python3 \
      python3-dev \
      rsync \
      sccache \
      tar \
      unzip \
      wayland-dev \
      wayland-protocols \
      xz \
      yasm \
      zip \
      zlib-dev \
      zstd && \
    update-ca-certificates

ENV PATH=/usr/lib/ninja-build/bin:/usr/lib/llvm22/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
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

RUN cat > /usr/local/bin/wfx-toolchain-report <<'EOF'
#!/bin/sh
set -eu
printf 'alpine='
cat /etc/alpine-release
printf 'arch='
apk --print-arch
clang-22 --version | sed -n '1p'
clang++-22 --version | sed -n '1p'
clang++-21 --version | sed -n '1p'
ld.lld --version | sed -n '1p'
mold --version
ninja --version
llvm-config-22 --version
rustc --version
cargo --version
cbindgen --version
sccache --version
EOF

RUN chmod +x /usr/local/bin/wfx-toolchain-report && wfx-toolchain-report

WORKDIR /work
