# How to Create a Custom Layer

Creating a custom dummy layer in Yocto/OpenEmbedded is straightforward. You only need a directory structure with a `conf/layer.conf` file, a `recipes-example/` folder, and a basic recipe.

Here is how to set up `meta-dummy` from scratch and register it in your build environment.

---

## Step 1: Create the Layer Directory Structure

Navigate to your Yocto source tree (usually alongside `meta` or in your main project folder) and create the standard directory layout:

```bash
mkdir -p meta-dummy/conf
mkdir -p meta-dummy/recipes-example/dummy
```

---

## Step 2: Create `conf/layer.conf`

Create `meta-dummy/conf/layer.conf` with the required BitBake configurations:

```bitbake
# We have a conf and classes directory, add to BBPATH
BBPATH .= ":${LAYERDIR}"

# We have recipes-* directories, add to BBFILES
BBFILES += "${LAYERDIR}/recipes-*/*/*.bb \
            ${LAYERDIR}/recipes-*/*/*.bbappend"

BBFILE_COLLECTIONS += "meta-dummy"
BBFILE_PATTERN_meta-dummy = "^${LAYERDIR}/"
BBFILE_PRIORITY_meta-dummy = "6"

LAYERVERSION_meta-dummy = "1"
LAYERSERIES_COMPAT_meta-dummy = "kirkstone scarthgap nanbield"
```

> **Note on `LAYERSERIES_COMPAT`:** Set this to the Yocto release codename you are building with (e.g., `scarthgap`, `kirkstone`, `mickledore`). BitBake will throw an error if your active release isn't in this list.

---

## Step 3: Add a Dummy Recipe

Create a simple hello-world style recipe at `meta-dummy/recipes-example/dummy/dummy-pkg_1.0.bb`:

```bitbake
SUMMARY = "Dummy test recipe"
DESCRIPTION = "A simple custom dummy recipe for testing Yocto builds"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = ""

S = "${WORKDIR}"

do_compile() {
    echo "Compiling dummy package..."
}

do_install() {
    install -d ${D}${bindir}
    echo '#!/bin/sh' > ${D}${bindir}/dummy-app
    echo 'echo "Hello from meta-dummy layer!"' >> ${D}${bindir}/dummy-app
    chmod +x ${D}${bindir}/dummy-app
}

FILES:${PN} += "${bindir}/dummy-app"
```

---

## Step 4: Add the Layer to Your Build

From your Yocto build directory (where `conf/bblayers.conf` lives), add the new layer using `bitbake-layers`:

```bash
bitbake-layers add-layer /path/to/meta-dummy
```

Alternatively, manually append the path to `BBLAYERS` inside `conf/bblayers.conf`:

```bitbake
BBLAYERS += " /path/to/meta-dummy "
```

---

## Step 5: Test and Build

1. **Verify BitBake recognizes the layer:**

   ```bash
   bitbake-layers show-layers
   ```

   *You should see `meta-dummy` listed with priority 6.*

2. **Build your dummy package:**

   ```bash
   bitbake dummy-pkg
   ```

3. **Add it to your image (Optional):**

   Add `IMAGE_INSTALL:append = " dummy-pkg"` to your `conf/local.conf` or image recipe to include it in your final rootfs.

---

## Shortcut Alternative: `bitbake-layers create-layer`

If you prefer BitBake to generate all the boilerplate files automatically, simply run:

```bash
bitbake-layers create-layer meta-dummy
bitbake-layers add-layer meta-dummy
```

This creates a fully configured layer skeleton with an example recipe in one command.
