#!/usr/bin/env bash
set -euo pipefail

# Build the development image using the existing devcontainer Dockerfile
# Usage: ./tools/docker-build.sh [image-name]

IMAGE_NAME=${1:-second-movement-dev}

echo "Building Docker image: ${IMAGE_NAME}:latest"
docker build -f .devcontainer/Dockerfile -t "${IMAGE_NAME}:latest" .

echo "\nDone. Run commands inside with:"
echo "  ./tools/run-in-docker.sh <cmd>..."
echo "or map ports via:"
echo "  DOCKER_ARGS='-p 8000:8000' ./tools/run-in-docker.sh <cmd>..."

