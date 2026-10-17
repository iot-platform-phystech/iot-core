FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libssl-dev \
    libbrotli-dev \
    libpq-dev \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt
RUN git clone https://github.com/drogonframework/drogon.git && \
    cd drogon && \
    git checkout v1.9.3 && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_POSTGRESQL=ON && \
    make -j$(nproc) && \
    make install && \
    ldconfig

WORKDIR /app
COPY . .
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libjsoncpp25 \
    uuid-runtime \
    zlib1g \
    libssl3 \
    libbrotli1 \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/iot-core .
COPY config.json .

EXPOSE 8000

CMD ["./iot-core"]