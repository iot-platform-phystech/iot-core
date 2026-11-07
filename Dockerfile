FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates cmake g++ make\
    libboost-system-dev \
    nlohmann-json3-dev \ 
    libcurl4-openssl-dev libssl-dev zlib1g-dev git \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 https://github.com/reo7sp/tgbot-cpp.git /tmp/tgbot-cpp \
 && cmake -S /tmp/tgbot-cpp -B /tmp/tgbot-cpp/build \
 && cmake --build /tmp/tgbot-cpp/build -j2 \
 && cmake --install /tmp/tgbot-cpp/build \
 && rm -rf /tmp/tgbot-cpp
 
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
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j2

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libjsoncpp25 \
    uuid-runtime \
    zlib1g \
    libssl3 \
    libbrotli1 \
    libpq5 ca-certificates \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/iot-core .
COPY --from=builder /usr/local/lib/ /usr/local/lib/
RUN ldconfig
ENV IOT_CORE_HOST=0.0.0.0

EXPOSE 8000

CMD ["./iot-core"]