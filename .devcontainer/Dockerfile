# Use Ubuntu 22.04 as base image (compatible with GCC-12 - Guernika)
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install basic tools and build dependencies
RUN apt-get update && apt-get install -y \
    software-properties-common \
    wget \
    curl \
    gnupg \
    build-essential \
    sudo \
    ninja-build \
    python3 \
    python3-pip \
    lsb-release \
    gdb \
    cppcheck \
    doxygen \
    graphviz \
    ccache \
    pkg-config \
    make \
    tar \
    git \
    ca-certificates \
    xz-utils \
    libssl-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Install GCC-12 and set as default
RUN apt-get update && apt-get install -y gcc-12 g++-12 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100 && \
    rm -rf /var/lib/apt/lists/*

# Install Valgrind 3.19.0 from source
RUN wget https://sourceware.org/pub/valgrind/valgrind-3.19.0.tar.bz2 && \
    tar -xjf valgrind-3.19.0.tar.bz2 && cd valgrind-3.19.0 && \
    ./configure --prefix=/usr/local && make -j$(nproc) && make install && \
    cd .. && rm -rf valgrind-3.19.0 valgrind-3.19.0.tar.bz2

# Install CMake latest from Kitware
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
    gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" && \
    apt-get update && apt-get install -y cmake && rm -rf /var/lib/apt/lists/*

# Install LLVM/Clang 20 for Jammy
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | \
    gpg --dearmor -o /etc/apt/trusted.gpg.d/llvm-snapshot.gpg && \
    add-apt-repository "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-20 main" && \
    apt-get update && apt-get install -y \
    clang-20 \
    clang++-20 \
    clang-tools-20 \
    clang-tidy-20 \
    clang-format-20 \
    clangd-20 \
    libc++-20-dev \
    libc++abi-20-dev \
    lld-20 \
    lldb-20 \
    && rm -rf /var/lib/apt/lists/*

# Set Clang-20 as default alternatives
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-20 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-20 100 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-20 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-20 100 && \
    update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-20 100

# Set working directory
WORKDIR /workspace

# Keep container running
CMD ["/bin/bash"]
