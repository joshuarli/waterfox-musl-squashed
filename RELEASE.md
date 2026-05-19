# Release Notes

## Waterfox musl minwayland x86_64

Current status: this repository has a manual GitHub Actions workflow for an
optimized, non-PGO `x86_64-alpine-linux-musl` minwayland release bundle. The
workflow builds inside Alpine edge Docker on `ubuntu-24.04`, packages only the
default `en-US` artifact, checks the staged runtime dependencies, runs a CLI
smoke, and uploads a gzip-compressed bundle with its reports.

Future TODO: add a PGO minwayland release build. The unresolved part is the
profile-generation strategy for this x86_64 workflow without depending on the
local QEMU proof path.
