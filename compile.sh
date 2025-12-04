#!/bin/bash

g++ src/cli/client.cpp -o src/cli/client
echo "Compiled Client!"
g++ src/srv/server.cpp -o src/srv/server
echo "Compiled Server!"
