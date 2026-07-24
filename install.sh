#!/bin/sh
set -e

INSTALL_DIR="${HOME}/.local"
BIN_DIR="${INSTALL_DIR}/bin"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== holycc Installer ==="
echo "Installing to: ${BIN_DIR}"
echo ""

mkdir -p "${BIN_DIR}"

echo "[1/3] Building..."
cmake -B "${PROJECT_DIR}/build" -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
cmake --build "${PROJECT_DIR}/build" 2>&1 | tail -3

echo ""
echo "[2/3] Installing binary..."
cp "${PROJECT_DIR}/build/src/holycc" "${BIN_DIR}/holycc"
chmod +x "${BIN_DIR}/holycc"

echo "[3/3] Copying runtime..."
mkdir -p "${INSTALL_DIR}/include" "${INSTALL_DIR}/lib"
cp "${PROJECT_DIR}/runtime/holyc_runtime.h" "${INSTALL_DIR}/include/"
cp "${PROJECT_DIR}/build/runtime/libholyc_runtime.a" "${INSTALL_DIR}/lib/"

echo ""
echo "=== Done ==="
echo ""

if ! echo "${PATH}" | grep -q "${BIN_DIR}"; then
    echo "Add this to your ~/.bashrc or ~/.profile:"
    echo ""
    echo "  export PATH=\"${BIN_DIR}:\$PATH\""
    echo ""
fi

echo "Usage: holycc program.HC"
echo ""
echo "Optional: Add 'alias hc=holycc' to your ~/.bashrc for quick access."
echo ""

HAS_HC=$(grep -c "alias hc=" "${HOME}/.bashrc" 2>/dev/null || true)
if [ "${HAS_HC}" = "0" ]; then
    echo "alias hc=holycc" >> "${HOME}/.bashrc"
    echo "Added 'hc' alias to ~/.bashrc"
fi

HAS_PATH=$(grep -c "${BIN_DIR}" "${HOME}/.bashrc" 2>/dev/null || true)
if [ "${HAS_PATH}" = "0" ]; then
    echo "export PATH=\"${BIN_DIR}:\$PATH\"" >> "${HOME}/.bashrc"
    echo "Added ${BIN_DIR} to PATH in ~/.bashrc"
fi
