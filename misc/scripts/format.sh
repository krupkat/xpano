#!/usr/bin/env bash

clang-format-22 -i `find xpano -name *.cc -or -name *.h`
clang-format-22 -i `find tests -name *.cc -or -name *.h`