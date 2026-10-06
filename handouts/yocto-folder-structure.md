# Yocto Folder Structure

Here is a breakdown of why Yocto uses multiple `meta-*` directories and how running `oe-init-build-env` constructs your `build/conf` folder from those meta layers.

---

## Why Are There Multiple `meta` Directories?

Yocto uses a **layered architecture** to keep metadata modular, organized, and easy to maintain or swap out:

- **`meta` (OpenEmbedded-Core / OE-Core):** The core foundation layer. It contains recipes for base C libraries, toolchains (GCC, LLVM), core utilities (BusyBox, systemd, system packages), and essential Linux build infrastructure. It is completely hardware-agnostic.
- **`meta-poky`:** The policy and branding layer for the **Poky** reference distribution. It defines default distribution settings, system naming conventions, release configurations, and user policies (such as setting `DISTRO = "poky"`).
- **`meta-yocto-bsp`:** The Board Support Package (BSP) layer for Yocto's official reference hardware and emulation boards (e.g., `qemux86-64`, `qemuarm`, `beaglebone-yocto`). It contains machine configurations, board-specific kernel definitions, and bootloader settings.
- **`meta-skeleton`:** An example/template layer designed as a reference for developers writing their own custom layers, recipes, or kernel module extensions from scratch.
- **`meta-selftest`:** A test layer used internally by BitBake and Yocto automated testing frameworks (`oe-selftest`) to verify that the build system and recipe parsers operate correctly.

---

## How `oe-init-build-env` Generates `build/conf`

When you execute `source oe-init-build-env ../build`, BitBake uses template configuration files embedded inside the `meta-poky` layer to generate your local project setup:

```text
+-----------------------------------------------------------------------+
|                            POKY REPOSITORY                            |
|                                                                       |
|  meta-poky/conf/templates/default/                                    |
|   ├── local.conf.sample   ─────────┐                                  |
|   ├── bblayers.conf.sample  ───────┼──────────────────┐               |
|   └── conf-notes.txt       ────────┼─────────┐        │               |
+------------------------------------+---------+--------+---------------+
                                     |         |        |
                                     | Copies  | Copies | Copies
                                     v         v        v
+-----------------------------------------------------------------------+
|                            BUILD DIRECTORY                            |
|                                                                       |
|  ~/build/conf/                                                        |
|   ├── local.conf  <─────────── [Build policies: CPU threads,          |
|   |                            target machine, image features]        |
|   |                                                                   |
|   ├── bblayers.conf <───────── [Active Layer Registry: Points         |
|   |                            BitBake to meta, meta-poky,            |
|   |                            and meta-yocto-bsp]                    |
|   |                                                                   |
|   ├── conf-notes.txt <──────── [Helpful CLI prompt messages displayed |
|   |                            when initializing environment]         |
|   |                                                                   |
|   └── templateconf.cfg <────── [Tracks which layer template path      |
|                                 was used to generate this conf]       |
+-----------------------------------------------------------------------+
```

---

## What Each Generated Config File Controls

- **`bblayers.conf`**: Lists every active `meta-*` directory path. BitBake scans these exact paths in sequence to parse `.bb` recipes and `.bbappend` files.
- **`local.conf`**: Controls your specific build settings (e.g., `MACHINE = "qemux86-64"`, `BB_NUMBER_THREADS`, parallel execution limits, and package management choices).
- **`templateconf.cfg`**: Contains the file path pointing back to `meta-poky/conf/templates/default/`, keeping track of which template set generated your `conf` directory.
