#!/bin/bash
set -e

# Tree build script for Horpkg
# Usage: ./build.sh [arm64-v8a|x86_64]

VERSION="2.1.1"
ARCH=${1:-arm64-v8a}

# Determine target
if [ "$ARCH" = "arm64-v8a" ]; then
    OHOS_ARCH="aarch64"
    TARGET="aarch64-linux-ohos"
elif [ "$ARCH" = "x86_64" ]; then
    OHOS_ARCH="x86_64"
    TARGET="x86_64-linux-ohos"
else
    echo "Error: Unsupported architecture: $ARCH"
    echo "Usage: $0 [arm64-v8a|x86_64]"
    exit 1
fi

echo "========================================"
echo "Building tree for $ARCH"
echo "========================================"

# Check environment
if [ -z "$OHOS_SDK_HOME" ]; then
    echo "Error: OHOS_SDK_HOME not set"
    exit 1
fi

# Setup environment
export CC="$OHOS_SDK_HOME/native/llvm/bin/clang"
export CFLAGS="--target=$TARGET --sysroot=$OHOS_SDK_HOME/native/sysroot -O2 -DLINUX"
export LDFLAGS="--target=$TARGET --sysroot=$OHOS_SDK_HOME/native/sysroot -s"

# Create build directory
BUILD_DIR="build-$ARCH"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Download source if not exists
if [ ! -f "tree-$VERSION.tgz" ]; then
    echo "Downloading tree-$VERSION.tgz..."
    curl -L -o "tree-$VERSION.tgz" \
        "https://mama.indstate.edu/users/ice/tree/src/tree-$VERSION.tgz"
fi

# Extract
echo "Extracting..."
tar xzf "tree-$VERSION.tgz"
cd "tree-$VERSION"

# Apply patches if any
if [ -d "../../patches" ]; then
    echo "Applying patches..."
    for patch in ../../patches/*.patch; do
        if [ -f "$patch" ]; then
            patch -p1 < "$patch"
        fi
    done
fi

# Build
echo "Building..."
make CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS"

# Create install directory
INSTALL_DIR="../install"
mkdir -p "$INSTALL_DIR/usr/bin"
mkdir -p "$INSTALL_DIR/usr/share/man/man1"

# Install
echo "Installing..."
cp tree "$INSTALL_DIR/usr/bin/"
cp doc/tree.1 "$INSTALL_DIR/usr/share/man/man1/"

# Create HNP package
echo "Creating HNP package..."
cd "$INSTALL_DIR"

# Create hnp.json
cat > hnp.json << EOF
{
    "type": "hnp-config",
    "name": "tree",
    "version": "$VERSION",
    "install": {}
}
EOF

# Package
cd ..
HNP_FILE="../tree-$VERSION-$ARCH.hnp"
rm -f "$HNP_FILE"
cd install
zip -r "$HNP_FILE" usr hnp.json

# Generate checksum
cd ..
sha256sum "../tree-$VERSION-$ARCH.hnp" > "../tree-$VERSION-$ARCH.hnp.sha256"

echo "========================================"
echo "Build complete!"
echo "Package: tree-$VERSION-$ARCH.hnp"
echo "SHA256:  tree-$VERSION-$ARCH.hnp.sha256"
echo "========================================"