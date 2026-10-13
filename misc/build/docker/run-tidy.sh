#!/usr/bin/env bash

docker build -f misc/build/docker/Dockerfile.tidy . -t xpanotidy --progress plain

docker run --rm xpanotidy
