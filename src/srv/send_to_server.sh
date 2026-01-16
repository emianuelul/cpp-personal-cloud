#!/bin/bash

set -e  # oprește la prima eroare

echo "🔨 Building server_exec in Docker container..."

docker run --rm \
  --platform=linux/amd64 \
  -v $(pwd)/../../:/work \
  -w /work/src/srv \
  centos:7 \
  bash -c "
    set -e
    echo '📦 Updating CentOS repos to vault...'
    sed -i 's|mirrorlist=|#mirrorlist=|g' /etc/yum.repos.d/CentOS-*.repo
    sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*.repo

    echo '📦 Installing centos-release-scl...'
    yum install -y centos-release-scl

    echo '📦 Fixing SCL repos...'
    sed -i 's|mirrorlist=|#mirrorlist=|g' /etc/yum.repos.d/CentOS-SCLo-*.repo
    sed -i 's|# baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-SCLo-*.repo
    sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-SCLo-*.repo

    echo '📦 Installing devtoolset-11 and dependencies...'
    yum install -y devtoolset-11 gcc gcc-c++ make sqlite-devel wget

    echo '📥 Downloading nlohmann/json...'
    mkdir -p /usr/local/include/nlohmann
    wget -q https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp -O /usr/local/include/nlohmann/json.hpp

    echo '🔧 Sourcing devtoolset-11...'
    source /opt/rh/devtoolset-11/enable
    echo 'GCC version:'
    gcc --version
    echo 'G++ version:'
    g++ --version

    echo '🔨 Compiling AES...'
    gcc -c -I../../include ../../include/aes.c -o aes.o

    echo '🔨 Compiling server...'
    g++ -std=c++20 \
      -I../../include \
      -I/usr/local/include \
      server.cpp aes.o \
      -o server_exec \
      -pthread -lsqlite3

    echo '✅ Compilation successful!'
    chmod 755 server_exec
    rm -f aes.o
  "

if [ ! -f server_exec ]; then
    echo "❌ Error: server_exec was not created!"
    exit 1
fi

echo "📤 Copying to remote server..."
scp server_exec emanuel.marin@10.100.0.30:~/

echo "🧹 Cleaning up..."
rm server_exec

echo "✅ Done!"