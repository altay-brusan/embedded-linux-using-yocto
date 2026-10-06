# How to Set Up Packages in a Custom Image

That is exactly the issue!

When `bitbake-layers create-layer` generates the boilerplate `example_0.1.bb` file, it creates a **stub template that doesn't actually produce or install any files into its package**.

By default, Yocto's build system (`bitbake`) skips generating an `.rpm` or `.deb` file for a recipe if that recipe's `${PN}` package ends up completely empty. Because no `example.rpm` package was produced, `dnf` threw the `No match for argument: example` error during `do_rootfs`.

---

## Step 1: Inspect `example_0.1.bb`

If you open `example_0.1.bb` right now, you'll likely see something like this:

```bitbake
SUMMARY = "bitbake-layers example recipe"
DESCRIPTION = "BitBake recipe list"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://example.txt"

S = "${WORKDIR}"
```

Notice that there is **no `do_install()` task** putting any files into `${D}` (the destination image directory).

---

## Step 2: Fix `example_0.1.bb`

Update `meta-mylayer/recipes-example/example/example_0.1.bb` so that it installs an executable or test script into `/usr/bin`:

```bitbake
SUMMARY = "Example test recipe for meta-mylayer"
DESCRIPTION = "A simple custom recipe that installs a dummy binary"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = ""

S = "${WORKDIR}"

do_compile() {
    :
}

do_install() {
    install -d ${D}${bindir}
    echo '#!/bin/sh' > ${D}${bindir}/my-example-app
    echo 'echo "Hello from meta-mylayer!"' >> ${D}${bindir}/my-example-app
    chmod +x ${D}${bindir}/my-example-app
}

FILES:${PN} += "${bindir}/my-example-app"
```

---

## Step 3: Rebuild

1. Build the recipe individually first to confirm it generates the package:

   ```bash
   bitbake example
   ```

2. Re-run your custom image build:

   ```bash
   bitbake my-custom-image
   ```

`dnf` will now find the generated `example.rpm` package in the build repo and build `my-custom-image` successfully!
