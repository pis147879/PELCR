# syntax=docker/dockerfile:1
FROM centos:latest
RUN cat /etc/centos-release
RUN cd /etc/yum.repos.d ;sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-*;sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*

RUN dnf -y update
RUN dnf -y install dnf-plugins-core
RUN dnf -y config-manager --set-enabled powertools

RUN dnf -y install git wget gcc gdb make openmpi openmpi-devel bison flex bison-devel flex-devel strace
COPY . /app
RUN cd /app ; rm -f Makefile ; ln -s Makefile.docker Makefile ; make
RUN make /app
RUN useradd -ms /bin/bash pelcr
RUN chown -R pelcr /app
USER pelcr
WORKDIR /app

CMD bash
