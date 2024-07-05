FROM ubuntu:22.04

RUN touch a
#install verilator
RUN apt-get -y update && apt-get -y install wget sudo help2man git perl python3 make autoconf g++ flex bison cmake ninja-build  zlib1g zlib1g-dev python3.10-venv lcov \
    && git clone https://github.com/verilator/verilator && cd verilator;git checkout v5.018 && autoconf && ./configure && make -j `nproc` && make install && cd .. && rm -rf verilator
