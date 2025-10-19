# Horpkg Index Repository

Official package index repository for Horpkg - HarmonyOS Recipe Package Manager.

## 📋 About

This repository contains the package index for Horpkg. It does NOT contain the actual packages, only metadata pointing to package repositories.

**Build from Source, Sign with Trust**

## 🗂️ Structure

- `index.json` - Main package index
- `categories.json` - Package categories
- `mirrors.json` - Mirror list
- `packages/` - Individual package metadata

## 📦 Available Packages

See [index.json](index.json) for the complete list.

Current package count: **1**

## 🤝 Contributing

To add a package:

1. Create a package repository (see [tree](https://github.com/horpkg-repo/tree) as example)
2. Build and publish releases
3. Submit a PR adding `packages/your-package.json`

### Package JSON Template
```json
{
  "name": "your-package",
  "version": "1.0.0",
  "description": "Package description",
  "repository": {
    "type": "git",
    "url": "https://github.com/horpkg-repo/your-package"
  },
  "architectures": ["arm64-v8a"],
  "sources": {
    "arm64-v8a": {
      "recipe": {
        "url": "https://raw.githubusercontent.com/horpkg-repo/your-package/main/your-package.recipe",
        "sha256": "..."
      },
      "upstream": {
        "url": "https://example.com/source.tar.gz",
        "sha256": "..."
      }
    }
  },
  "binaries": {
    "arm64-v8a": {
      "public_hnp": {
        "url": "https://github.com/horpkg-repo/your-package/releases/download/v1.0.0/your-package-1.0.0-arm64.hnp",
        "sha256": "..."
      }
    }
  }
}
```

## 🔗 Links

- [Horpkg CLI](https://github.com/horpkg/horpkg)
- [Package Template](https://github.com/horpkg-repo/template)
- [Documentation](https://horpkg.org/docs)

## 📄 License

MIT License