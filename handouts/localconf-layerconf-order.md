# `local.conf`, `bblayers.conf`, and Build Order

That is a key insight into how BitBake operates. Your mental model of how `bblayers.conf` and `local.conf` interact with the layers is spot on, but understanding where `core-image-minimal` actually lives in that structure clarifies how BitBake processes it.

---

## Where does `core-image-minimal` come from?

`core-image-minimal` is **not an executable or a binary** before the build; it is a **recipe file** named `core-image-minimal.bb`.

It lives inside the core layer at this exact file path:

```text
poky/meta/recipes-core/images/core-image-minimal.bb
```

When you run `bitbake core-image-minimal`:

1. BitBake reads `bblayers.conf` to know where to search (e.g., `meta`, `meta-poky`, `meta-yocto-bsp`).
2. It searches those layers and finds `poky/meta/recipes-core/images/core-image-minimal.bb`.
3. It parses that recipe, which specifies what packages are needed for a minimal booting system (such as `busybox`, `init-scripts`, and key base libraries).
4. It applies your global settings from `local.conf` (e.g., `MACHINE = "qemux86-64"`).
5. It compiles everything inside the `tmp/work/` directory.
6. Once compiled, it packs all those output binaries into the final image files located at `build/tmp/deploy/images/qemux86-64/`.

---

## Visualizing the Workflow

```text
1. RECIPE LOCATION
   poky/meta/recipes-core/images/core-image-minimal.bb
                               │
                               ▼
2. CONFIGURATION OVERLAYS
   bblayers.conf  ──────►  [ Tells BitBake which layers to parse ]
   local.conf     ──────►  [ Sets MACHINE = "qemux86-64", threads, etc. ]
                               │
                               ▼
3. EXECUTION & COMPILATION
   bitbake core-image-minimal
   ├── BitBake builds toolchain & target packages
   └── Work directory: build/tmp/work/
                               │
                               ▼
4. OUTPUT / DEPLOYMENT
   build/tmp/deploy/images/qemux86-64/
   ├── core-image-minimal-qemux86-64.rootfs.ext4   <-- Target Filesystem Image
   ├── core-image-minimal-qemux86-64.rootfs.wic    <-- Disk Image
   └── bzImage                                     <-- Linux Kernel
```

## Summary

- **The Recipe:** `poky/meta/recipes-core/images/core-image-minimal.bb` (the instructions)
- **The Output Image:** `build/tmp/deploy/images/qemux86-64/core-image-minimal-qemux86-64.rootfs.ext4` (the final built filesystem)
