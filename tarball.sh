#! /bin/bash

cd "$(dirname "$0")"
git archive -o "osavul-$1.tar.gz" --prefix="osavul-$1/" "v$1"
