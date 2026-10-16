#!/usr/bin/env bash

docker build -f misc/build/docker/Dockerfile.appimage . -t xpanobuilder --progress plain

docker run --rm -v ./install:/install xpanobuilder \
  bash -c "cp *.AppImage /install"
