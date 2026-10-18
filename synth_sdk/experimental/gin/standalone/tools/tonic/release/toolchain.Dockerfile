FROM ubuntu:22.04@sha256:b1066385161d28ddf6bc7e7b28a9170eec11484c821d1a5150d176cbde41d7f7

ARG DEBIAN_FRONTEND=noninteractive
ARG UBUNTU_SNAPSHOT=20261001T000000Z

RUN --mount=type=secret,id=extra_ca,required=false,mode=0444 \
    set -eux; \
    snap="https://snapshot.ubuntu.com/ubuntu/${UBUNTU_SNAPSHOT}/"; \
    if [ -s /run/secrets/extra_ca ]; then \
      echo 'Acquire::https::CAInfo "/run/secrets/extra_ca";' > /etc/apt/apt.conf.d/99extra-ca; \
    else \
      apt-get update; \
      apt-get install -y --no-install-recommends ca-certificates; \
    fi; \
    sed -i "s#http://archive.ubuntu.com/ubuntu/#${snap}#g; s#http://security.ubuntu.com/ubuntu/#${snap}#g" \
        /etc/apt/sources.list; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
      build-essential ca-certificates curl xz-utils bzip2 gzip tar \
      python3 cmake file patch gnupg \
      bison flex texinfo gawk libgmp-dev libmpfr-dev libmpc-dev zlib1g-dev; \
    rm -f /etc/apt/apt.conf.d/99extra-ca; \
    rm -rf /var/lib/apt/lists/*; \
    dpkg-query -W -f='${Package}=${Version}\n' > /etc/gin-image-packages.txt

LABEL org.gin.image="gin-bfin-dev" \
      org.gin.ubuntu-snapshot="${UBUNTU_SNAPSHOT}" \
      org.gin.purpose="Blackfin toolchain; no TE firmware inside"

WORKDIR /workspace
ENV BFIN_PREFIX=/workspace/.toolchain
ENV PATH=/workspace/.toolchain/bin:${PATH}
CMD ["bash"]
