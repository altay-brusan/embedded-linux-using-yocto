# How Packages Are Integrated into Images

`inherit core-image` pulls in the **infrastructure** and **build logic** for creating an image (file system generation, package manager handling, rootfs creation tasks), but it does **not** automatically install application software beyond the absolute bare minimum needed to boot.

Here is how it breaks down:

---

## What `inherit core-image` Actually Provides

1. **Rootfs Creation Tasks:** It wires up BitBake tasks like `do_rootfs`, image compression, and partition creation (`.wic` image generation).
2. **Core Feature Logic:** It gives you access to the `IMAGE_FEATURES` variable (e.g., `ssh-server-dropbear`, `read-only-rootfs`, `debug-tweaks`).
3. **Default Boot Packages:** By default, `core-image` automatically includes `packagegroup-core-boot`, which contains the bare essential system requirements:
   - Init system (`sysvinit` or `systemd`)
   - BusyBox / core utilities
   - C runtime library (`glibc` or `musl`)
   - `/dev` manager (`udev` / `mdev`)

---

## Why You Add Packages to `IMAGE_INSTALL`

If you create a completely blank custom image recipe with just `inherit core-image`, you get a bootable shell, but **no extra tools, drivers, or custom apps**.

You use `IMAGE_INSTALL` to explicitly list the extra software packages you want included in the final root filesystem:

```bitbake
SUMMARY = "My Custom CM3 Image"
LICENSE = "MIT"

# 1. Bring in image building logic & bare-minimum boot software
inherit core-image

# 2. Add high-level image features
IMAGE_FEATURES += "ssh-server-dropbear"

# 3. Add explicit packages you need in your rootfs
IMAGE_INSTALL += " \
    i2c-tools \
    can-utils \
    my-custom-app \
"
```

---

## Key Variables Summary

| Variable | Purpose | Example |
| --- | --- | --- |
| **`inherit core-image`** | Brings in standard OE image construction rules and task handling. | `inherit core-image` |
| **`IMAGE_FEATURES`** | Toggles pre-configured system features/capabilities. | `IMAGE_FEATURES += "ssh-server-dropbear splash"` |
| **`IMAGE_INSTALL`** | Explicit list of individual `.bb` package names to install in rootfs. | `IMAGE_INSTALL += "i2c-tools dummy-pkg"` |
