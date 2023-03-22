FROM debian:11
ENV DEBIAN_FRONTEND=noninteractive

VOLUME /HSO/experiment

WORKDIR /HSO
RUN mkdir -p /HSO/patches

# Install depedencies
RUN apt-get update &&                          \
    apt-get install -y --no-install-recommends \
    bison                                      \
    build-essential                            \
    ca-certificates                            \
    gawk                                       \
    git                                        \
    python3                                    \
    python3-pip                                \
    rsync                                      \
    zlib1g                                     \
    zlib1g-dev                                 \
    && rm -rf /var/lib/apt/lists/*


# Clone glibc
RUN git clone --depth=1                  \
    https://sourceware.org/git/glibc.git \
    glibc &&                             \
    cd glibc &&                          \
    git checkout db9b47e9f996bbdb831580ff7343542a017c80ee
WORKDIR /HSO/glibc

# Apply my patch
COPY src/c/patches/* /HSO/patches/
RUN git apply /HSO/patches/*.patch

# Build and test glibc
WORKDIR /HSO/glibc-build
RUN /HSO/glibc/configure --prefix=/usr && make -j12
# RUN make check -j12

WORKDIR /HSO

COPY requirements.txt /HSO
RUN pip install -r /HSO/requirements.txt

COPY src/ src/
WORKDIR /HSO/src/c
RUN make GLIBC=/HSO/glibc-build all glibc && \
    ln -s /HSO/src/c/HSO-c /HSO/HSO-c && \
    ln -s /HSO/src/c/HSO-c-glibc /HSO/HSO-c-glibc

WORKDIR /HSO/experiment
ENTRYPOINT [ "/HSO/src/jobs.py" ]
