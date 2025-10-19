# Tree for HarmonyOS

Tree command for Horpkg - recursive directory listing.

## 📦 Package Information

- **Name**: tree
- **Version**: 2.1.1
- **Category**: Utilities
- **License**: GPL-2.0

## 📥 Installation
```bash
horpkg install tree
```

## 🛠️ Build from Source
```bash
horpkg install --from-source tree
```

## 📋 Description

Tree is a recursive directory listing command that produces a depth-indented listing of files, which is colorized ala dircolors if the LS_COLORS environment variable is set and output is to tty.

## 🔗 Upstream

- Homepage: https://mama.indstate.edu/users/ice/tree/
- Source: https://mama.indstate.edu/users/ice/tree/src/tree-2.1.1.tgz

## 📄 Files Provided

### Commands
- `tree` - Main executable

### Man Pages
- `tree.1` - Manual page

## 🏗️ Build Instructions

This package is built using GitHub Actions. See [.github/workflows/build.yml](.github/workflows/build.yml).

Manual build:
```bash
./build.sh arm64-v8a
# or
./build.sh x86_64
```

## 📦 Releases

Releases are automatically published to GitHub Releases with:
- `tree-VERSION-ARCH.hnp` - Binary HNP package
- `tree-VERSION-ARCH.hnp.sha256` - Checksum file

## 🤝 Contributing

Contributions welcome! Please:
1. Fork this repository
2. Create a feature branch
3. Submit a pull request

## 📄 License

GPL-2.0 (same as upstream tree)