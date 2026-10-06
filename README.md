# Embedded Linux Using Yocto

These are my study notes for the Udemy course **"Embedded Linux Using Yocto"**.

The repository has two parts:

- **The course's own lecture notes**, organized by day as the course presents them.
- **My own handouts**: side notes I wrote to understand topics in more depth, or to record fixes and tips I found while working through the course.

## Repository layout

```
.
├── lecture notes/   # Course lecture notes, exercises and example code (day1 … day16)
└── handouts/        # My own deeper notes on selected topics
```

## Lecture notes

The `lecture notes/` folder follows the course day by day. Each day contains numbered `notes.txt` files, and some days also have example source code and a `challenge.txt` exercise.

| Day | Topics |
|-----|--------|
| [day1](lecture%20notes/day1) | Embedded Linux basics, what Yocto is, Poky, metadata, OpenEmbedded, BitBake, building and running images in QEMU |
| [day2](lecture%20notes/day2) | Yocto terminology, Poky source, build directory, build workflow, images, saving disk space |
| [day3](lecture%20notes/day3) | BeagleBone Black: building an image, the boot process (ROM, SPL, U-Boot), SD card setup, serial console |
| [day4](lecture%20notes/day4) | Yocto releases, the meta-ti layer |
| [day5](lecture%20notes/day5) | Raspberry Pi 3 / Zero W: meta-raspberrypi, the boot process, flashing, SSH access, `IMAGE_ROOTFS_EXTRA_SPACE` |
| [day6](lecture%20notes/day6) | BitBake operators, creating a layer |
| [day7](lecture%20notes/day7) | Image recipes, `IMAGE_FEATURES`, other images |
| [day8](lecture%20notes/day8) | Recipe basics, a recipe for a C program |
| [day9](lecture%20notes/day9) | Logs and debugging, more recipe examples |
| [day10](lecture%20notes/day10) | Git fetcher: branches, tags, local and private repositories, patching sources |
| [day11](lecture%20notes/day11) | Packaging: splitting files, `PACKAGES`, `FILES`, the installed-vs-shipped error |
| [day12](lecture%20notes/day12) | Static and dynamic libraries in recipes |
| [day13](lecture%20notes/day13) | Dependencies: `DEPENDS`, `RDEPENDS`, sharing files between recipes, dependency graphs, `noexec` |
| [day14](lecture%20notes/day14) | Autotools and CMake recipes |
| [day15](lecture%20notes/day15) | CMake in depth, devshell |
| [day16](lecture%20notes/day16) | `FILESPATH`, a custom splash screen, `.bbappend` files |

## My handouts

The `handouts/` folder holds the side notes I wrote while following the course.

**Environment setup**
- [Set up the build environment with Python 3.11](handouts/setup-env-with-python-3-11.md)
- [Python 3.11 setup: bugfix](handouts/setup-env-with-python-3-11-bugfix.md)
- [Copy an image from WSL to Windows and flash it](handouts/how-to-copy-image-from-wsl-to-windows.md)

**Yocto structure and configuration**
- [Yocto folder structure](handouts/yocto-folder-structure.md)
- [Folder names inside `recipes-*`](handouts/folder-name-in-yocto.md)
- [`local.conf`, `bblayers.conf`, and build order](handouts/localconf-layerconf-order.md)
- [Where does each setting belong?](handouts/yocto-where-config-belongs.md)

**Layers**
- [Create a custom layer](handouts/how-to-create-custom-layer.md)
- [Create a custom layer from a template](handouts/how-to-create-custom-layer-from-template.md)
- [`yocto-check-layer` command example](handouts/yocto-check-layer-command-example.md)

**Images and packages**
- [Create an image with a custom layer](handouts/how-to-create-image.md)
- [Set up packages in a custom image](handouts/how-to-setup-packages-in-custom-image.md)
- [How packages are integrated into images](handouts/how-packages-integrated-into-images.md)
- [`IMAGE_INSTALL` and `IMAGE_FEATURES`](handouts/image-install-and-image-feature.md)
- [Linux packages: core vs. peripheral](handouts/linux-packages-core-pheripheral.md)

**Beyond the course**
- [Yocto + Qt 6: building a QML dashboard recipe (Q&A)](handouts/yocto-qt-recipe-qa.md)
- [The Linux display stack, explained](handouts/linux-display-stack-explained.md)

## Note

The lecture notes come from the course and are kept here for my personal study. The handouts are my own work.
