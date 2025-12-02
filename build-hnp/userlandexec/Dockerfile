FROM --platform=linux/arm64/v8 alpine:3.19

RUN apk add --no-cache \
    bash \
    gdb strace \
    build-base binutils coreutils findutils util-linux procps

WORKDIR /workspace
