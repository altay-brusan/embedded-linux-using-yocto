# `yocto-check-layer` Command Example

The failure occurs because `yocto-check-layer` evaluates the signature generation of the entire layer collection (`world` target), and `omxplayer` in `meta-raspberrypi` depends on **`libssh`**, which is missing from your active `BBLAYERS` path. `libssh` resides in **`meta-openembedded/meta-oe`**.

---

## Solution

Pass the required dependency layer (`meta-openembedded/meta-oe`) into `yocto-check-layer` using the `--dependency` (or `-d`) flag so the parser can resolve `libssh`:

```bash
yocto-check-layer --dependency /home/altay/source/meta-openembedded/meta-oe -- meta-mylayer
```

If `meta-raspberrypi` requires other sub-layers from `meta-openembedded` (such as `meta-python` or `meta-networking`), include them as well:

```bash
yocto-check-layer \
  --dependency /home/altay/source/meta-openembedded/meta-oe \
  --dependency /home/altay/source/meta-openembedded/meta-python \
  --dependency /home/altay/source/meta-openembedded/meta-networking \
  -- meta-mylayer
```

---

## Alternative Fix (If You Aren't Using `omxplayer`)

If `omxplayer` is not needed in your target distribution, you can mask it out in your `conf/bblayers.conf` or `conf/local.conf` so BitBake skips parsing it entirely during layer checks:

```bitbake
BBMASK += "meta-raspberrypi/recipes-multimedia/omxplayer/"
```
