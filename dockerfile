# syntax=docker/dockerfile:1
FROM centos:latest
RUN cat /etc/centos-release
RUN cd /etc/yum.repos.d ;sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-*;sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*

RUN dnf -y update
RUN dnf -y install dnf-plugins-core
RUN dnf -y config-manager --set-enabled powertools

RUN dnf -y install git wget gcc gdb make openmpi openmpi-devel bison flex bison-devel flex-devel strace
RUN yum -y debuginfo-install libevent-2.1.8-5.el8.x86_64 openmpi-4.1.1-2.el8.x86_64 openmpi-devel-4.1.1-2.el8.x86_64 openssl-libs-1.1.1k-5.el8_5.x86_64 zlib-1.2.11-17.el8.x86_64
COPY . /app
RUN cd /app ; rm -f Makefile ; ln -s Makefile.docker Makefile ; make
RUN make /app
RUN useradd -ms /bin/bash pelcr
RUN chown -R pelcr /app
USER pelcr
WORKDIR /app

CMD bash
