#!/bin/bash

docker run --rm -v $(pwd)/../../:/work -w /work/src/srv alpine:latest sh -c "
  apk add --no-cache g++ nlohmann-json sqlite-dev sqlite-static build-base && \
  g++ -std=c++20 -static \
  -I../../include \
  -I/usr/include \
  server.cpp \
  -o server_exec \
  -pthread -lsqlite3
" && \
scp server_exec emanuel.marin@10.100.0.30:~/ && \
rm server_exec