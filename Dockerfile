FROM ubuntu:22.04
LABEL project="FlexoGraph"
LABEL version="0.1.0"
LABEL maintainer="Puneet Mehrotra"
LABEL description="Build environment for FlexoGraph"
ENV HOME=/root
SHELL [ "/bin/bash" , "-c" ]

RUN apt-get update && apt-get install ca-certificates -y
RUN apt-get --no-install-recommends -y install build-essential cmake git libboost-all-dev libomp-dev wget gdb autotools-dev autoconf automake libtool gnupg #needed for Intel TBB
RUN wget -O- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB | gpg --dearmor | tee /usr/share/keyrings/oneapi-archive-keyring.gpg > /dev/null
RUN echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" | tee /etc/apt/sources.list.d/oneAPI.list
RUN apt-get update && apt apt-get install intel-oneapi-base-toolkit

#Environment variables
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++

#Build and install FlexoGraph
# COPY . /FlexoGraph
ENV GRAPH_PROJECT_DIR="/FlexoGraph"
WORKDIR /FlexoGraph

#Release build
RUN rm -rf /FlexoGraph/build/release
#RUN mkdir -p build/release && cd build/release && cmake ../.. && make -j

# #Debug build
# RUN mkdir -p build/debug && cd build/debug && cmake -DCMAKE_BUILD_TYPE=Debug ../.. && make -j

#check if build is successful
#RUN cd build/release/test && ./test_adj_list
CMD sleep infinity
