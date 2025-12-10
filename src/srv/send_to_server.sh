#!/bin/bash

docker run --rm -v $(pwd):/work -w /work alpine:latest sh -c "
  apk add --no-cache g++ && \
  g++ -std=c++14 -static -o server_exec server.cpp -pthread
" && \
scp server_exec emanuel.marin@10.100.0.30:~/ && \
rm server_exec