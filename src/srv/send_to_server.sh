#!/bin/bash

docker run --rm -v $(pwd)/../../:/work -w /work/src/srv alpine:latest sh -c "
  apk add --no-cache g++ gcc nlohmann-json sqlite-dev sqlite-static build-base && \
  gcc -c -I../../include ../../include/aes.c -o aes.o && \
  chmod 666 aes.o && \
  g++ -std=c++20 -static \
  -I../../include \
  -I/usr/include \
  server.cpp aes.o \
  -o server_exec \
  -pthread -lsqlite3 && \
  chmod 777 server_exec && \
  rm -f aes.o
" && \
scp server_exec emanuel.marin@10.100.0.30:~/ && \
rm server_exec