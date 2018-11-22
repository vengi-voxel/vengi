FROM debian:buster as builder
MAINTAINER Martin Gerhardy <martin.gerhardy@gmail.com>

ENV DEBIAN_FRONTEND noninteractive

RUN echo 'Acquire::HTTP::Proxy "http://172.17.0.1:3142";' >> /etc/apt/apt.conf.d/01proxy \
 && echo 'Acquire::HTTPS::Proxy "false";' >> /etc/apt/apt.conf.d/01proxy

RUN apt-get update -q && apt-get install -y cmake g++ pkg-config \
	opencl-c-headers libcurl4-openssl-dev postgresql-server-dev-11 \
	libsdl2-dev zlib1g-dev libuv1-dev uuid-dev ocl-icd-libopencl1

COPY contrib /tmp/@ROOT_PROJECT_NAME@/contrib
COPY tools /tmp/@ROOT_PROJECT_NAME@/tools
COPY cmake /tmp/@ROOT_PROJECT_NAME@/cmake
COPY data /tmp/@ROOT_PROJECT_NAME@/data
COPY src /tmp/@ROOT_PROJECT_NAME@/src
COPY CMakeLists.txt /tmp/@ROOT_PROJECT_NAME@/

RUN mkdir /tmp/@ROOT_PROJECT_NAME@/build
RUN cmake --version
RUN cmake -H/tmp/@ROOT_PROJECT_NAME@ -B/tmp/@ROOT_PROJECT_NAME@/build -DUNITTESTS=OFF -DVISUALTESTS=OFF -DTOOLS=OFF -DCLIENT=OFF -DSERVER=ON -DRCON=OFF -DCMAKE_INSTALL_PREFIX=installation -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
RUN cat /tmp/@ROOT_PROJECT_NAME@/build/engine-config.h
RUN make -C /tmp/@ROOT_PROJECT_NAME@/build -j $(nproc) server

FROM debian:buster

RUN echo 'Acquire::HTTP::Proxy "http://172.17.0.1:3142";' >> /etc/apt/apt.conf.d/01proxy \
 && echo 'Acquire::HTTPS::Proxy "false";' >> /etc/apt/apt.conf.d/01proxy

RUN apt-get update -q && \
	apt-get install -y libatomic1 libcurl4 zlib1g libuv1 libpq5 libsdl2-2.0.0 \
	ocl-icd-libopencl1 uuid-runtime && \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*

COPY --from=builder /tmp/@ROOT_PROJECT_NAME@/build/@PROJECT_NAME@ /opt/@ROOT_PROJECT_NAME@/

EXPOSE @SERVER_PORT@

WORKDIR /opt/@ROOT_PROJECT_NAME@
ENTRYPOINT ./@ROOT_PROJECT_NAME@-@PROJECT_NAME@
