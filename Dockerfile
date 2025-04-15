FROM ubuntu:16.04

RUN apt-get update \
    && DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata \
    build-essential flex bison bc ccache ecj fastjar file g++ gawk uuid-dev \
    gettext git java-propose-classpath libelf-dev libncurses5-dev \
    libncursesw5-dev libssl-dev python python2.7-dev python3 unzip wget \
    python3-setuptools python3-dev rsync subversion \
    swig time xsltproc zlib1g-dev sudo \
    && apt-get clean \
    # Set "docker" as root password
    echo "root:docker" | chpasswd && \
    # Adds docker user
    useradd -m docker && \
    echo "docker:docker" | chpasswd && adduser docker sudo && \
    echo "docker ALL=(ALL:ALL) NOPASSWD: ALL" | sudo tee /etc/sudoers.d/docker

RUN git config --global user.email "liqili2005@gmail.com"
RUN git config --global user.name "Kunkka Li"

# Download and install OpenSSL 1.0.2
RUN wget https://www.openssl.org/source/openssl-1.0.2u.tar.gz && \
    tar -xvzf openssl-1.0.2u.tar.gz && \
    cd openssl-1.0.2u && \
    ./config --prefix=/usr/local/openssl-1.0.2 --openssldir=/usr/local/openssl-1.0.2 && \
    make && \
    make install

ENV PATH="/usr/local/openssl-1.0.2/bin:${PATH}"
ENV LD_LIBRARY_PATH="/usr/local/openssl-1.0.2/lib:${LD_LIBRARY_PATH}"
RUN ln -sf /bin/bash /bin/sh
# Set docker as default
USER docker:docker

CMD ["bash"]