# How to Create an Image with a Custom Layer

To build a target Linux image (like `core-image-minimal`) using your new custom layer, you have three options depending on your goal:

---

## Option 1: Include Your Recipe in an Existing Image (Simplest)

If you just want your custom layer's package (e.g., `example`) built into your Raspberry Pi image:

1. **Add your layer to `bblayers.conf`** (if you haven't already):

   ```bash
   bitbake-layers add-layer /home/altay/source/meta-mylayer
   ```

2. **Add your package to `local.conf`:**

   Open `build/conf/local.conf` and append:

   ```bitbake
   IMAGE_INSTALL:append = " example"
   ```

   *(Replace `example` with the name of the recipe in `meta-mylayer/recipes-example/`.)*

3. **Build the image:**

   ```bash
   bitbake core-image-minimal
   ```

---

## Option 2: Create a Custom Image Recipe in Your Layer

If you want to define a standalone image recipe dedicated to your layer:

1. **Create an `images` folder in your layer:**

   ```bash
   mkdir -p /home/altay/source/meta-mylayer/recipes-core/images
   ```

2. **Create the image recipe file** (`meta-mylayer/recipes-core/images/my-custom-image.bb`):

   ```bitbake
   SUMMARY = "Custom image for Raspberry Pi CM3"
   DESCRIPTION = "A minimal Linux image including custom packages from meta-mylayer."
   LICENSE = "MIT"

   # Inherit standard core image definitions
   inherit core-image

   # Core system requirements
   IMAGE_FEATURES += "splash ssh-server-dropbear"

   # Add custom packages here
   IMAGE_INSTALL += " \
       packagegroup-core-boot \
       example \
   "
   ```

3. **Build your new custom image:**

   ```bash
   bitbake my-custom-image
   ```

---

## Option 3: Customize an Image via a `.bbappend` File

If you want to modify `core-image-minimal` directly from inside `meta-mylayer` without altering `local.conf`:

1. **Create a `.bbappend` path:**

   ```bash
   mkdir -p /home/altay/source/meta-mylayer/recipes-core/images
   ```

2. **Create `meta-mylayer/recipes-core/images/core-image-minimal.bbappend`:**

   ```bitbake
   IMAGE_INSTALL:append = " example"
   ```

3. **Build normally:**

   ```bash
   bitbake core-image-minimal
   ```
