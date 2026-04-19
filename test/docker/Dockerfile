FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++-14 \
    cmake \
    make \
    libgtest-dev \
    google-mock \
    && rm -rf /var/lib/apt/lists/*

# Build GTest from source (ubuntu package doesn't include prebuilt libs on 24.04)
WORKDIR /tmp/gtest-build
RUN cmake /usr/src/googletest \
    -DCMAKE_CXX_COMPILER=g++-14 \
    -DCMAKE_CXX_FLAGS="-std=c++20 -fno-rtti -fno-exceptions" \
    -DBUILD_SHARED_LIBS=ON \
    && cmake --build . --parallel \
    && cmake --install . \
    && ldconfig \
    && rm -rf /tmp/gtest-build

WORKDIR /src

COPY CMakeLists.txt .
COPY include/ include/
COPY test/ test/

CMD ["bash", "-c", "cmake -B build -DCMAKE_CXX_COMPILER=g++-14 -DPIPEPP_CORE_BUILD_TESTS=ON -DCMAKE_CXX_FLAGS='-std=c++20 -fno-rtti -fno-exceptions' && cmake --build build --parallel && cd build && ctest --output-on-failure"]