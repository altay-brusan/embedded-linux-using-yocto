# How to Create a Custom Layer from a Template

Yes, Yocto provides built-in tools to auto-generate layer and recipe templates instantly without writing boilerplate code manually.

---

## 1. Auto-Generate a Layer Template (`bitbake-layers create-layer`)

BitBake includes a command that creates a complete layer directory structure, `conf/layer.conf`, a README, and an example recipe automatically:

```bash
bitbake-layers create-layer meta-dummy
```

This generates:

```text
meta-dummy/
├── COPYING.MIT
├── README
├── conf/
│   └── layer.conf
└── recipes-example/
    └── example/
        └── example_0.1.bb
```

To enable it right away:

```bash
bitbake-layers add-layer meta-dummy
```

---

## 2. Auto-Generate a Recipe Template (`recipetool create`)

If you want to create a recipe for existing software (from local source code or a Git URL/tarball), Yocto provides `recipetool`. It inspects your build system (Autotools, CMake, Makefile, Python setup.py, Cargo, etc.), fetches license files, generates checksums, and writes the entire recipe for you:

### From a local source directory

```bash
recipetool create -o meta-dummy/recipes-example/my-app/my-app.bb /path/to/source/code
```

### From a URL or Git repository

```bash
recipetool create -o meta-dummy/recipes-example/my-app/my-app.bb https://github.com/user/my-app/archive/v1.0.tar.gz
```

`recipetool` handles the `LIC_FILES_CHKSUM`, dependencies, `SRC_URI`, and build steps automatically.
