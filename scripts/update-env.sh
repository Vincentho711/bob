#!/usr/bin/env bash
# Admin script: pull a new bob-env image from GHCR, convert to Apptainer .sif,
# and deploy to shared storage with an atomic symlink swap.
#
# Usage: update-env.sh <image-tag>
# Example: update-env.sh sha-a1b2c3d
#
# Requires: apptainer, write access to NFS_IMAGE_DIR
# Run on a node with NFS mounted and internet access to GHCR.

set -euo pipefail

IMAGE_TAG="${1:?Usage: $0 <image-tag>}"
IMAGE_REPO="ghcr.io/vincentho711/bob-env"
NFS_IMAGE_DIR="${BOB_NFS_IMAGE_DIR:-/nfs/images}"
SIF_NAME="bob-env-${IMAGE_TAG}.sif"
SIF_PATH="${NFS_IMAGE_DIR}/${SIF_NAME}"
SYMLINK="${NFS_IMAGE_DIR}/bob-env-current.sif"

echo "==> Pulling ${IMAGE_REPO}:${IMAGE_TAG} from GHCR..."
apptainer pull "${SIF_PATH}" "docker://${IMAGE_REPO}:${IMAGE_TAG}"

echo "==> Updating symlink: ${SYMLINK} -> ${SIF_NAME}"
ln -sfn "${SIF_NAME}" "${SYMLINK}"

echo "==> Done. Active image: $(readlink -f "${SYMLINK}")"
echo ""
echo "Verify with:"
echo "  apptainer exec ${SYMLINK} verilator --version"
echo "  apptainer exec ${SYMLINK} gcc --version"
