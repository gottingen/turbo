FROM centos:centos7.5.1804

# author lijippy@163.comm
MAINTAINER jeff.li

COPY environment_linux.yaml /env/environment_linux.yaml
COPY build.sh /env/build.sh

RUN /env/build.sh

RUN rm -r /env