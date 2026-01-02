#!/bin/bash

docker run --rm -v $(pwd)/../../:/work -w /work/src/srv alpine:latest sh -c "
  apk add --no-cache g++ nlohmann-json && \
  g++ -std=c++20 -static -lsqlite3\
  -I../../include \
  -I/usr/include \
  -o server_exec server.cpp -pthread
" && \
scp server_exec emanuel.marin@10.100.0.30:~/ && \
rm server_exec