# Yocto: Where Does Each Setting Belong?

A study note on one of the most confusing parts of Yocto: the same line often "works" in several files, so how do you decide where it should go? It starts with the layer hierarchy, gives a simple rule, sorts the real settings from my Raspberry Pi dashboard build, and ends with the target layout (a custom distro).

---

## 0. Abbreviations and terms

| Term | Meaning |
|---|---|
| **Yocto** | The Yocto Project: tools and metadata for building custom embedded Linux |
| **Poky** | The Yocto Project's reference distribution and build system (BitBake + core layers) |
| **BitBake** | The build engine that reads recipes and config files and runs tasks |
| **OE** | OpenEmbedded, the build framework and layer collection Yocto is based on |
| **conf** | Configuration file (`.conf`) |
| **distro** | Distribution: the *policy* of the system you build |
| **machine** | The *hardware* you build for |
| **recipe** (`.bb`) | Instructions to build one piece of software |
| **bbappend** (`.bbappend`) | A file in your layer that modifies someone else's recipe without editing it |
| **image recipe** | A recipe whose output is a whole filesystem image and which lists what goes into it |
| **layer** (`meta-*`) | A folder of recipes and config, kept in version control |
| **QPA** | Qt Platform Abstraction (Qt's display plugin layer, e.g. eglfs) |
| **eglfs** | EGL Full Screen (Qt plugin that draws full screen with no desktop) |
| **UART** | Universal Asynchronous Receiver-Transmitter (the serial console) |
| **DTBO** | Device Tree Blob Overlay (a patch to the hardware description) |
| **KMS** | Kernel Mode Setting |
| **ptest** | Package test (on-target test suites; Yocto feature) |
| **CPU** / **RAM** | Central Processing Unit / Random Access Memory |
| **VM** | Virtual Machine (e.g. Ubuntu in VirtualBox) |

---

## 1. The problem

When we were iterating fast, almost everything went into `conf/local.conf`, and it all worked. So why move it?

Because **`local.conf` is not part of the product.** It lives in the build directory, is created per developer, and is normally **not in version control**. If the definition of your product lives there, a colleague (or a CI server, or you in six months) who clones your layers gets a **different image**.

---

## 2. The hierarchy: from general to specific

```text
  bitbake.conf          Poky's base defaults                    NEVER edit
       │
  distro conf           WHAT KIND of system                     meta-*/conf/distro/*.conf
       │                (init system, features, policy)
  machine conf          WHAT HARDWARE                           meta-*/conf/machine/*.conf
       │                (CPU, bootloader, device tree, UART)
  image recipe          WHAT'S IN THIS IMAGE                    meta-*/recipes-*/images/*.bb
       │                (package list, image features)
  recipe / bbappend     HOW ONE PACKAGE BUILDS                  *.bb / *.bbappend
       │
  local.conf            THIS DEVELOPER, THIS PC                 build/conf/local.conf
                        (cores, paths, mirrors, today's choices)   (not version controlled)
```

Think of it as concentric circles of *who the setting is about*:

```text
 ┌───────────────────────────────────────────────────────────┐
 │  DISTRO: every product of this kind                        │
 │   ┌───────────────────────────────────────────────────┐    │
 │   │  MACHINE: this board                               │    │
 │   │   ┌───────────────────────────────────────────┐    │    │
 │   │   │  IMAGE: this particular image             │    │    │
 │   │   │   ┌───────────────────────────────────┐   │    │    │
 │   │   │   │  RECIPE / BBAPPEND: one package    │   │    │    │
 │   │   │   └───────────────────────────────────┘   │    │    │
 │   │   └───────────────────────────────────────────┘    │    │
 │   └───────────────────────────────────────────────────┘    │
 └───────────────────────────────────────────────────────────┘
   local.conf sits OUTSIDE the product: it's about the person and PC building it.
```

---

## 3. The one-question rule

> **If I deleted `local.conf` and rebuilt, would the product be different?**
>
> If **yes**, that setting is in the wrong place.

Expanded into a decision table:

| Ask yourself | Put it in |
|---|---|
| Is it a fact about the **hardware**? | machine conf |
| Is it **product policy** (init system, features, licensing)? | distro conf |
| Does it describe **what goes into this image**? | image recipe |
| Does it change **how one package builds**? | recipe or `.bbappend` |
| Is it about **my PC** (cores, RAM, download/cache paths, mirrors)? | `local.conf` |
| Is it a **temporary experiment**? | `local.conf`, then promote it to the right place once it works |

As a flow chart:

```text
                         new setting
                              │
             is it about my build PC / my paths?
                 │ yes                     │ no
                 ▼                         ▼
            local.conf          is it about the hardware?
                                  │ yes          │ no
                                  ▼              ▼
                            machine conf   does it affect only one package?
                                             │ yes            │ no
                                             ▼                ▼
                                    recipe / .bbappend   is it "what's in this image"?
                                                           │ yes        │ no
                                                           ▼            ▼
                                                     image recipe   distro conf
                                                                   (product policy)
```

---

## 4. Why "local.conf wins" (and when it doesn't)

It's common to hear "local.conf overrides everything." In practice that's mostly true, but the reason is worth knowing because it explains surprises.

**Parse order.** BitBake's `bitbake.conf` reads config files roughly in this order:

```text
 1. conf/local.conf            ◄── read FIRST
 2. conf/machine/${MACHINE}.conf
 3. conf/distro/${DISTRO}.conf
 4. layer.conf files, classes ...
 5. recipes and .bbappends     ◄── read LAST (per recipe)
```

So `local.conf` is actually read **before** machine and distro. It "wins" only because well-written machine and distro files use **weak assignments**:

| Operator | Meaning | Who wins? |
|---|---|---|
| `VAR = "x"` | Hard set | The **last** file parsed |
| `VAR ?= "x"` | Set only if not already set | The **first** file parsed (so `local.conf` beats it) |
| `VAR ??= "x"` | Weakest default, used only if nothing else sets it | Anyone else |
| `VAR:append = " x"` / `:prepend` | Add text, applied at the end | Everyone's appends are combined |
| `VAR:remove = "x"` | Remove a word, applied after everything | Remove always wins |
| `VAR:pn-<recipe> = "x"` | Apply only when building that recipe | Lets a global file target one recipe |

Consequences:

- If a distro file does a hard `INIT_MANAGER = "systemd"`, your `INIT_MANAGER = "sysvinit"` in `local.conf` **loses**, because distro is parsed later.
- A recipe's own `VAR = "x"` beats `local.conf` for that recipe. That's why you see `PACKAGECONFIG:append:pn-qtbase` in `local.conf`: the `:append` + `:pn-` combination reaches into one recipe.

**To see the final value and where it came from:**

```bash
bitbake-getvar -r qtbase PACKAGECONFIG       # newer releases
bitbake -e qtbase | grep -B20 '^PACKAGECONFIG='   # any release; shows the history
```

---

## 5. Sorting my actual settings

### Belongs in a **distro conf**: product policy

```bitbake
INIT_MANAGER = "sysvinit"                         # which init system the product uses
DISTRO_FEATURES:remove = "x11 wayland ptest"      # no desktop, no on-target test suites
QT_QPA_DEFAULT_PLATFORM = "eglfs"                 # Qt apps go full screen by default
NO_RECOMMENDATIONS = "1"                          # don't pull in "recommended" extras
DISABLE_SPLASH = "1"                              # boot look-and-feel
DISABLE_RPI_BOOT_LOGO = "1"
CMDLINE_DEBUG = "quiet vt.global_cursor_default=0"  # quiet boot, no blinking cursor
```

Why: these decide **what kind of system** this is, whatever board it runs on.

> Gray area: `DISABLE_SPLASH`, `DISABLE_RPI_BOOT_LOGO` and `CMDLINE_DEBUG` are Raspberry Pi specific variables (from `meta-raspberrypi`). They express product *look and feel*, so a distro conf is reasonable; putting them in your custom machine conf is also defensible. What matters is that they're in **your layer**, not in `local.conf`.

### Belongs in a **machine conf** in my layer: hardware facts

```bitbake
VC4DTBO = "vc4-kms-v3d"                               # use the full KMS graphics driver overlay
ENABLE_UART = "1"                                     # serial console on the GPIO header
MACHINE_FEATURES:remove = "wifi bluetooth"            # this board variant doesn't use them
MACHINE_EXTRA_RRECOMMENDS:remove = "linux-firmware-rpidistro-..."   # drop the matching firmware
```

Why: they describe **this board and how it's wired**.

### Correctly in **local.conf**: properties of my PC

```bitbake
BB_NUMBER_THREADS = "1"        # how many BitBake tasks at once (low: small VirtualBox VM)
PARALLEL_MAKE = "-j 2"         # compiler jobs per task
MACHINE ??= "raspberrypi3-64"  # which board I'm building today (debatable, see below)
```

Also typical here: `DL_DIR` (download cache), `SSTATE_DIR` (sstate cache), `SSTATE_MIRRORS`, proxy settings.

> `MACHINE` is "debatable" because it's a *choice per build*, not a fact about the product. Keeping it in `local.conf` lets the same layers build for several boards.

### Already in the right place: a **bbappend**

```text
meta-dashboard/recipes-qt/qt6/qtshadertools_git.bbappend    # cstdint compile fix
```

A build fix for **one package**: exactly what a `.bbappend` is for.

### One more to move: the Qt graphics flags

From the display-stack work:

```bitbake
PACKAGECONFIG:append:pn-qtbase = " eglfs gles2 kms gbm"
```

This changes how **one package** (qtbase) builds, so the cleanest home is a bbappend:

```bitbake
# meta-dashboard/recipes-qt/qt6/qtbase_%.bbappend
PACKAGECONFIG:append = " eglfs gles2 kms gbm"
```

(The `%` matches any qtbase version.) It could also stay in the distro conf with the `:pn-qtbase` form if you see it as product policy. Either is fine; `local.conf` is not.

---

## 6. The target layout: a custom distro

The professional shape: your own layer holds the distro, the machine tweaks, the image and the app recipe. `local.conf` shrinks to "which board, which distro, and my PC settings".

```text
meta-dashboard/
├── conf/
│   ├── layer.conf
│   ├── distro/
│   │   └── dashboard.conf              ◄ product policy
│   └── machine/
│       └── dashboard-rpi3.conf         ◄ (optional) hardware tweaks
├── recipes-core/
│   └── images/
│       └── dashboard-image.bb          ◄ what goes into the image
├── recipes-qt/
│   └── qt6/
│       ├── qtbase_%.bbappend           ◄ eglfs gles2 kms gbm
│       └── qtshadertools_git.bbappend  ◄ cstdint fix
└── recipes-apps/
    └── smarthome-dashboard/
        ├── smarthome-dashboard_git.bb
        └── files/
            └── smarthome-dashboard.init
```

### The distro file

```bitbake
# meta-dashboard/conf/distro/dashboard.conf
require conf/distro/poky.conf          # start from Poky's defaults

DISTRO = "dashboard"
DISTRO_NAME = "Smart Home Dashboard OS"
DISTRO_VERSION = "1.0"

INIT_MANAGER = "sysvinit"
DISTRO_FEATURES:remove = "x11 wayland ptest"
NO_RECOMMENDATIONS = "1"
QT_QPA_DEFAULT_PLATFORM = "eglfs"

DISABLE_SPLASH = "1"
DISABLE_RPI_BOOT_LOGO = "1"
CMDLINE_DEBUG = "quiet vt.global_cursor_default=0"
```

### The machine tweaks: two options

**Option A: a custom machine that builds on the board's file**

```bitbake
# meta-dashboard/conf/machine/dashboard-rpi3.conf
require conf/machine/raspberrypi3-64.conf

VC4DTBO = "vc4-kms-v3d"
ENABLE_UART = "1"
MACHINE_FEATURES:remove = "wifi bluetooth"
MACHINE_EXTRA_RRECOMMENDS:remove = "linux-firmware-rpidistro-..."
```

> Caveat: with a new machine name, recipes that use overrides like `:raspberrypi3-64` won't match automatically. You may need to add the base name to `MACHINEOVERRIDES` (e.g. `MACHINEOVERRIDES =. "raspberrypi3-64:"`). Check with `bitbake -e | grep ^MACHINEOVERRIDES=`.

**Option B: keep the stock machine, use machine-specific overrides in the distro**

```bitbake
# in dashboard.conf
ENABLE_UART:raspberrypi3-64 = "1"
VC4DTBO:raspberrypi3-64 = "vc4-kms-v3d"
```

Simpler, but mixes hardware facts into the policy file. Option A is cleaner when you have more than a couple of hardware lines.

### What's left in local.conf

```bitbake
MACHINE ?= "raspberrypi3-64"     # or "dashboard-rpi3" with Option A
DISTRO  ?= "dashboard"

BB_NUMBER_THREADS = "1"
PARALLEL_MAKE = "-j 2"
# DL_DIR / SSTATE_DIR / mirrors as needed for this PC
```

And the test that proves it's right: **a colleague clones the layers, writes only these lines, runs `bitbake dashboard-image`, and gets the same image.**

---

## 7. Quick reference

| File | Answers the question | Version controlled? | Examples |
|---|---|---|---|
| `bitbake.conf` | Poky's base defaults | (part of Poky) | never edit |
| distro conf | What kind of system? | Yes, in your layer | `INIT_MANAGER`, `DISTRO_FEATURES`, `NO_RECOMMENDATIONS` |
| machine conf | What hardware? | Yes, in your layer | `VC4DTBO`, `ENABLE_UART`, `MACHINE_FEATURES` |
| image recipe | What's in this image? | Yes | `IMAGE_INSTALL`, `IMAGE_FEATURES` |
| recipe / bbappend | How does one package build? | Yes | `PACKAGECONFIG`, patches, `DEPENDS` |
| `local.conf` | Who is building, on what PC, today? | **No** | `BB_NUMBER_THREADS`, `PARALLEL_MAKE`, `DL_DIR`, `MACHINE`, `DISTRO` |

**The rule to remember:** `local.conf` is a scratchpad and a description of your PC. Anything that defines the product moves into a layer.
