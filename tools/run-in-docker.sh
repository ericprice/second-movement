#!/usr/bin/env bash
set -euo pipefail

# Run a command inside the dev image, mounting the repo at /work
# Usage: ./tools/run-in-docker.sh [--image name] <cmd> [args...]
#
# Examples:
#   ./tools/run-in-docker.sh make BOARD=sensorwatch_red DISPLAY=classic
#   ./tools/run-in-docker.sh emmake make BOARD=sensorwatch_red DISPLAY=classic
#   DOCKER_ARGS='-p 8000:8000' ./tools/run-in-docker.sh python -m http.server -d build-sim 8000

IMAGE_NAME="second-movement-dev:latest"

if [[ "${1:-}" == "--image" ]]; then
  shift
  IMAGE_NAME="$1"
  shift
fi

DOCKER_ARGS=${DOCKER_ARGS:-}

exec docker run --rm -it \
  ${DOCKER_ARGS} \
  -v "$(pwd)":/work \
  -w /work \
  "${IMAGE_NAME}" \
  "$@"

