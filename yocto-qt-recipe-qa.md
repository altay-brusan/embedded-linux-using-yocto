# Yocto + Qt 6: Building a QML Dashboard Recipe, Explained as Questions and Answers

A study note built from a recipe I wrote for a Qt 6 QML smart-home dashboard. Read it top to bottom to follow the build from source to image, or jump to any question.

---

## Part 1: The recipe

```bitbake
SUMMARY = "Smart Home Dashboard in Qt 6 QML"
DESCRIPTION = "A simple smart home dashboard built with Qt 6 QML"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "git://github.com/allankoechke/SmartHomeDashboardNew.git;protocol=https;branch=master \
           file://smarthome-dashboard.init \
"

# Pinned deliberately. AUTOREV re-polls GitHub on every parse, defeats sstate
# reuse and makes the image unreproducible - a different dashboard each build.
# Bump this when you actually want the new upstream commit.
SRCREV = "1274e0045705207ce7e8b5fec9a8a28a4e32a968"

S = "${WORKDIR}/git"

DEPENDS += "qtbase qtdeclarative qtdeclarative-native"

inherit qt6-cmake update-rc.d

# Start last (99) so udev has created /dev/dri/card0 before eglfs opens it.
INITSCRIPT_NAME = "smarthome-dashboard"
INITSCRIPT_PARAMS = "start 99 2 3 4 5 . stop 20 0 1 6 ."

do_install:append() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/smarthome-dashboard.init \
        ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}

FILES:${PN} += "${sysconfdir}/init.d/${INITSCRIPT_NAME}"

# The QML runtime is loaded at runtime via dlopen, so it is invisible to the
# shared-library dependency scanner and has to be named explicitly. Doubly so
# here because local.conf sets NO_RECOMMENDATIONS = "1", which means nothing
# arrives via RRECOMMENDS.
RDEPENDS:${PN} += " \
    qtbase-plugins \
    qtdeclarative-qmlplugins \
    mesa-megadriver \
"
```

### What each block does

| Block | Purpose | Used at which stage |
|---|---|---|
| `SUMMARY`, `DESCRIPTION`, `LICENSE`, `LIC_FILES_CHKSUM` | Metadata and license check | Parse / QA |
| `SRC_URI` | Where my app source and my init script come from | `do_fetch`, `do_unpack` |
| `SRCREV` | Exactly which Git commit to build | `do_fetch` + task hashing |
| `S` | Where the unpacked source lives | All build tasks |
| `DEPENDS` | What must be built *before* my app can compile | Before `do_configure` |
| `inherit qt6-cmake` | *How* to configure and compile a Qt 6 CMake project | `do_configure`, `do_compile`, `do_install` |
| `inherit update-rc.d` + `INITSCRIPT_*` | Register my app as a SysV boot service | Packaging + first boot |
| `do_install:append` | Put my init script into the staging root `${D}` | `do_install` |
| `FILES:${PN}` | Which package the init script ships in | `do_package` |
| `RDEPENDS:${PN}` | What must also be on the device for my app to *run* | Image creation |

### The whole life of this recipe in one picture

```text
 do_fetch      Download my app (git) + init script into DL_DIR
     |
 do_unpack     Unpack into WORKDIR  (source ends up in S = WORKDIR/git)
     |
 [DEPENDS]     Wait until qtbase, qtdeclarative, qtdeclarative-native are built,
     |         then copy their headers/libs/tools into MY private sysroots
     |
 do_configure  qt6-cmake runs CMake; find_package(Qt6 ...) finds Qt in my sysroot
 do_compile    Cross-compile my app for the target CPU
 do_install    Copy binary + init script into ${D}  (a fake root filesystem on the PC)
     |
 do_package    Split ${D} into packages using FILES:${PN}  -> .ipk / .deb / .rpm
     |
 [image]       Image recipe installs my package + its RDEPENDS into one rootfs,
               then formats it (ext4, wic, ...) -> the file I flash to the board
```

---

## Part 2: Qt basics

### Q1. What are `qtbase` and `qtdeclarative`?

They are two of Qt's source repositories. Qt is split into many repositories, and each one contains a group of modules.

| | `qtbase` | `qtdeclarative` |
|---|---|---|
| Purpose | Core, lowest-level Qt APIs and classic widgets | The declarative UI framework (QML / Qt Quick) |
| Key modules | QtCore, QtGui, QtWidgets, QtNetwork, QtSql, QtTest | QtQml, QtQuick, QtQuickControls, QtQmlModels |
| UI style | Imperative C++ desktop UIs (QWidgets) | Declarative, fluid, touch-friendly UIs (QtQuick) |
| Dependency | Depends on no other Qt repository | Depends on `qtbase`, must be built after it |

`qtbase` also provides the core build tools (`moc`, `rcc`, CMake config files) that `qtdeclarative` then uses to build its QML engine and tools.

### Q2. What is `qtdeclarative-native`?

In Yocto, the `-native` suffix means **"build this to run on my host PC"**, not on the target board.

So `qtdeclarative-native` is `qtdeclarative` compiled for my x86_64 build machine. It provides the host-side tools needed while building a QML app:

- **qmlcachegen / QML compiler**: pre-compiles QML/JavaScript so the app starts faster on the target.
- **qmlimportscanner**: finds which QML modules the app imports, so deployment knows what to include.
- **qmllint**: checks QML syntax during the build.

The target board can't run these during the build (it hasn't even been flashed yet), so the host must.

```bitbake
DEPENDS  += "qtbase-native qtdeclarative-native"   # tools that run on the PC during the build
RDEPENDS:${PN} += "qtbase qtdeclarative"           # libraries that must exist on the device
```

### Q3. Instead of `DEPENDS += "qtbase qtdeclarative"`, can I list modules: `DEPENDS = "QtCore, QtGui, QtQuick, ..."`?

**No.** There are two different levels:

1. **Yocto (BitBake) works with recipes.** It knows `qtbase` and `qtdeclarative`, not `QtCore` or `QtQuick`. One recipe builds a whole repository with all its modules.
2. **CMake works with modules.** Inside the app's `CMakeLists.txt`, you pick the exact modules:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)
target_link_libraries(my_app PRIVATE Qt6::Core Qt6::Gui Qt6::Quick)
```

| System | Works with | Example |
|---|---|---|
| BitBake (`.bb`) | Recipes (repositories) | `DEPENDS += "qtbase qtdeclarative"` |
| CMake | Individual modules | `find_package(Qt6 COMPONENTS Core Quick)` |

> ⚠️ In BitBake lists, separate items with **spaces**, never commas.

---

## Part 3: Fetching and dependencies

### Q4. After `SRC_URI` fetches my source, what happens? Where is Qt? How does `DEPENDS` fit in?

Key idea: **`DEPENDS` does not tell my app where Qt is. It tells BitBake to wait until Qt is built.**

1. **Fetch and unpack (`SRC_URI`)**: BitBake downloads my dashboard code from GitHub and unpacks it into my recipe's own work directory (`WORKDIR`). At this point it knows nothing about Qt.
2. **The pause (`DEPENDS`)**: Before my app is allowed to configure, BitBake checks whether `qtbase`, `qtdeclarative` and `qtdeclarative-native` are built. If not, it builds them first, using *their own* recipes.
3. **Injection into my sandbox**: When they're done, BitBake copies (hard-links) their headers, libraries, CMake config files and tools into **my recipe's private sysroots**:
   - `recipe-sysroot` = target-side files (ARM headers and libs to link against)
   - `recipe-sysroot-native` = host-side tools (`moc`, `qmlcachegen`, `cmake`, ...)
4. **Configure (`do_configure`)**: CMake runs. Yocto has already pointed it at my sysroots, so `find_package(Qt6 ...)` finds Qt there.

```text
 [My recipe]
   |-- 1. Fetch my code via SRC_URI  -> WORKDIR
   |-- 2. DEPENDS += "qtbase ..."    -> PAUSE, build Qt recipes first
   |-- 3. RESUME                     -> Qt files linked into MY sysroots
   '-- 4. do_configure               -> CMake finds Qt inside my sysroots
```

### Q5. How does Yocto know where to download Qt from? I never put Qt in my `SRC_URI`, and I already have Qt installed on my PC.

My recipe doesn't know. **The layer system does.**

1. **Layers are registered globally.** In `conf/bblayers.conf`:

   ```bitbake
   BBLAYERS ?= " \
     /path/to/poky/meta \
     /path/to/meta-openembedded/meta-oe \
     /path/to/meta-qt6 \
   "
   ```

   BitBake scans every recipe in every listed layer before building anything.

2. **Qt recipes already exist in `meta-qt6`.** It contains files like `qtbase_git.bb` and `qtdeclarative_git.bb`, and *they* have their own `SRC_URI` pointing to the official Qt Git repositories.

3. **`DEPENDS` is a name lookup.** When I write `DEPENDS += "qtbase"`, BitBake looks up a recipe called `qtbase` in its index, finds it in `meta-qt6`, and builds it with that recipe's `SRC_URI`.

**It never uses the Qt installed on my Ubuntu host.** That is deliberate: the build must not depend on whatever happens to be installed on a particular PC ("works on my machine" bugs).

### Q6. If I already have `DEPENDS`, why do I need `inherit qt6-cmake`?

They do different jobs:

- **`DEPENDS` = the "what"**: make sure Qt is built and its files are in my sandbox.
- **`inherit` = the "how"**: bring in the build logic (the `do_configure`, `do_compile`, `do_install` steps) that knows how to run CMake for a Qt 6 project, cross-compile for the target, and point CMake at the sysroots and host tools.

Without `inherit qt6-cmake`, Qt would be sitting in my sandbox but my recipe would use generic build rules that don't know how to find it or how to cross-compile a Qt app.

> **Note on `qtdeclarative-native`:** The `qt6-cmake` class adds `qtbase-native` for you. Whether it also adds `qtdeclarative-native` depends on the `meta-qt6` version, so keeping `qtdeclarative-native` in `DEPENDS` explicitly (as my recipe does) is the safe choice for a QML app. Listing it twice does no harm.

### Q7. Is this a correct summary?
> *`DEPENDS` = "Poky, leave my recipe, build my dependencies with their own settings, then come back." `inherit qt6-cmake` = "use the CMake setup from building Qt here in my recipe so I inherit its paths and variables."*

The `DEPENDS` part is **100% correct.**

The `inherit` part needs one small correction: `inherit` does **not** change or reuse how Qt was built. Qt was already built in its own recipe. `inherit qt6-cmake` configures **my app's** build so it finds and links against the Qt files now in my sandbox.

Kitchen version:
- **`DEPENDS`**: "Fetch the Qt ingredients from the other recipes and put them on my counter."
- **`inherit qt6-cmake`**: "Now cook *my* dish with the Qt cooking instructions, using what's on the counter."

---

## Part 4: Boot service and install

### Q8. What does this comment mean? `# Start last (99) so udev has created /dev/dri/card0 before eglfs opens it.`

At boot, SysV init starts services in order of priority number (00 to 99).

- **udev** scans hardware and creates device files in `/dev`. For the GPU it creates `/dev/dri/card0`.
- **eglfs** is the Qt platform plugin that draws the QML UI directly on the screen, with no X11 or Wayland.

If the dashboard started too early, `/dev/dri/card0` might not exist yet and the app would fail to open the screen. Priority **99** means "start as late as possible", after the graphics devices are ready.

```bitbake
INITSCRIPT_NAME   = "smarthome-dashboard"
INITSCRIPT_PARAMS = "start 99 2 3 4 5 . stop 20 0 1 6 ."
```

Read it as: *start at priority 99 in runlevels 2, 3, 4, 5; stop at priority 20 in runlevels 0 (halt), 1 (single-user) and 6 (reboot).*

### Q9. My target filesystem type differs from my host's. Is this how Yocto handles it? It builds a fake Linux root (`/usr`, `/etc`, `/bin`, ...) on the host, `${D}` points into it, and at the end Yocto formats it as the target filesystem type and packs everything in.

**Yes, that model is accurate.** It happens in two stages.

**Stage 1: `${D}`, a per-recipe fake root on the host.**
`${D}` (destination) is an empty directory on my PC that looks like a mini Linux root. When I run:

```bash
install -d ${D}${sysconfdir}/init.d
install -m 0755 ${WORKDIR}/smarthome-dashboard.init ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
```

it expands to something like
`.../tmp/work/<arch>/smarthome-dashboard/<version>/image/etc/init.d/smarthome-dashboard`.
I'm just arranging files into normal Linux paths inside a folder on my PC. (`${sysconfdir}` is `/etc`.)

**Stage 2: the image.**
1. Each recipe's `${D}` is split into packages (`.ipk`, `.deb` or `.rpm`).
2. The image recipe (e.g. `core-image-minimal`) creates one big **rootfs** directory on the host and installs all the needed packages into it: base OS, Qt, my app.
3. Based on `IMAGE_FSTYPES` (e.g. `ext4`, `wic`), host tools like `mkfs.ext4` create the final image file and copy the rootfs into it.

The host's own filesystem type doesn't matter; everything is ordinary files in folders until the very last formatting step.

### Q10. What is `FILES:${PN} += "${sysconfdir}/init.d/${INITSCRIPT_NAME}"`?

It says: **"this file belongs in my main package."**

- `${PN}` = package name, here `smarthome-dashboard`.
- `FILES:${PN}` = the list of paths that go into that package.
- `+=` adds to the default list instead of replacing it.

During packaging, Yocto checks that every file in `${D}` was assigned to some package. If a file isn't claimed, the build stops with a QA error:

```text
ERROR: QA Issue: smarthome-dashboard: Files/directories were installed but not shipped in any package:
  /etc/init.d/smarthome-dashboard
```

This is a safety net against loose files sneaking into (or silently missing from) the image.

> In practice `/etc/init.d/*` is often already in the default `FILES` list, so this line may be redundant, but stating it explicitly is harmless and documents intent.

### Q11. Is `FILES:${PN}` only used at packaging/imaging time, not when other recipes build against mine?

**Correct.**

- When another recipe has my recipe in its `DEPENDS`, it receives my **sysroot** output (headers, `.so` libraries, CMake/pkg-config files). `FILES` plays no part in that.
- `FILES:${PN}` is used in `do_package` to split `${D}` into packages, and those packages are what the image later installs.

| Stage | Uses `FILES:${PN}`? |
|---|---|
| Another recipe compiling against mine (`DEPENDS`) | No |
| Packaging my recipe (`do_package`) | Yes |
| Building the final image | Indirectly, through the packages |

---

## Part 5: Multiple recipes in one layer

### Q12. If my layer has several recipes (Qt app, network service, C++ security service) that depend on each other, do they all share one sandboxed root during compile and install?

**No, the opposite.** Modern Yocto uses **per-recipe sysroots**: every recipe gets its own private sandbox.

- While `network` compiles, its sysroot contains only what *it* declared in `DEPENDS`. It can't see Qt.
- While `qt-app` compiles, it sees `network`'s files **only** if `network` is in `qt-app`'s `DEPENDS`.

How files cross from one recipe to another:

1. `network` runs `do_install` and puts its `.so` and `.h` files into its own `${D}`.
2. `do_populate_sysroot` copies the development-relevant part (headers, libraries, config files) into a staging area for `network`.
3. Because `qt-app` has `DEPENDS += "network"`, BitBake links exactly those files into `qt-app`'s `recipe-sysroot` before it configures.

So `qt-app` sees `network`, but not `security`, unless `security` is also listed in `DEPENDS`.

**Where they finally meet:** only in the image step, when all packages are installed into one rootfs.

**Why this design?** It prevents hidden dependencies. In a shared sandbox, an app might compile only because another recipe happened to build first. Then one day the order changes and the build breaks. Per-recipe sysroots make builds reproducible.

---

## Part 6: Runtime dependencies

### Q13. What does the `RDEPENDS` block with the `dlopen` comment mean?

```bitbake
RDEPENDS:${PN} += " \
    qtbase-plugins \
    qtdeclarative-qmlplugins \
    mesa-megadriver \
"
```

**The problem:** after packaging, Yocto scans my binary to see which shared libraries it links (e.g. `libQt6Core.so`) and automatically adds those packages as runtime dependencies. But Qt loads plugins (platform plugins, QML modules, graphics) at runtime with **`dlopen()`**. They are not linked into the binary, so the scanner can't see them.

Without this block, the build succeeds but the device shows a black screen or the app crashes.

| Package | What it provides | Error you'd get without it |
|---|---|---|
| `qtbase-plugins` | Platform plugins such as `eglfs` | `Could not find the Qt platform plugin "eglfs"` |
| `qtdeclarative-qmlplugins` | QML modules: QtQuick, Controls, Layouts ... | `module "QtQuick.Controls" is not installed` |
| `mesa-megadriver` | OpenGL/EGL GPU drivers | eglfs can't initialise the GPU |

**About `NO_RECOMMENDATIONS = "1"`:** recipe authors often list such plugins under `RRECOMMENDS` ("install unless told not to"). My `local.conf` disables all recommendations to keep the image small, so anything the app really needs at runtime must be in `RDEPENDS`.

**Rule of thumb:**
- `DEPENDS` = needed to **build** (on the PC)
- `RDEPENDS` = needed to **run** (on the device)

---

## Part 7: Caching and reproducibility

### Q14. What is sstate?

**sstate = Shared State**, Yocto's build cache. It's why the first build takes hours and an unchanged rebuild takes about a minute.

For each task (`do_configure`, `do_compile`, `do_install`, `do_populate_sysroot`, `do_package`, ...):

1. **Hash the inputs**: source revision, recipe text, variables, compiler, and the hashes of its dependencies, combined into one signature.
2. **Look in `sstate-cache`** for an output with that signature.
3. **Hit**: reuse the stored result, skip the work.
   **Miss**: run the task and save the result to the cache for next time.

**Why "shared":** the cache can live on a server. A team or CI server publishes it (via `SSTATE_MIRRORS`), so a developer's build downloads already-built results instead of compiling Qt and the kernel locally.

Useful command:

```bash
bitbake -c cleansstate smarthome-dashboard   # drop this recipe's cache; it rebuilds from scratch next time
```

### Q15. Where is `AUTOREV` defined? Is it a default Poky variable?

Yes. It's defined in Poky's core config file **`meta/conf/bitbake.conf`**, roughly:

```bitbake
AUTOREV = "${@bb.fetch2.get_autorev(d)}"
SRCREV ??= "INVALID"
```

`AUTOREV` is a small function call that asks the Git server, at **parse time**, for the newest commit on the branch in `SRC_URI`. Writing `SRCREV = "${AUTOREV}"` means "use whatever the branch tip is right now."

Note the second line: the **default** `SRCREV` is `"INVALID"`. If a Git recipe sets no `SRCREV` at all, the build doesn't "always pull the branch"; it **fails** with an error asking for a valid `SRCREV`. You must choose either a fixed commit or `${AUTOREV}`.

### Q16. How does fetching work with a pinned `SRCREV`? Is pinning just a temporary trick to avoid rebuilds while debugging, which I remove afterwards?

That last part is the misconception. **Pinning is the normal, permanent state. Don't remove it after a successful build.**

**With a pinned SRCREV:**
1. `do_fetch` checks the Git mirror in `DL_DIR` (`downloads/git2/...`).
2. If that commit is already there, there's **no network access**. If not, it fetches once.
3. The commit hash is part of the task signature, so if nothing else changes, every task is an **sstate hit**.
4. Building the same `SRCREV` today, next month or on CI gives the **same** dashboard.

**With `${AUTOREV}`:**
1. **Every parse** contacts GitHub to ask for the branch tip (slower, and fails offline).
2. If upstream hasn't moved, the hash is the same and sstate is still reused, so it does **not** rebuild everything every time.
3. If upstream **has** moved, the hash changes, so my app and everything depending on it rebuild, and the image changes **with no record** in my layer of which commit went in.

| | Pinned `SRCREV` | `${AUTOREV}` |
|---|---|---|
| Network on each parse | No | Yes |
| Which commit is built | Exactly the one written down | Whatever is on the branch now |
| Reproducible | Yes | No |
| sstate reuse | Always, until I change it | Lost whenever upstream moves |
| Typical use | Releases, CI, normal work | Quick local development of my own code |

**The workflow:** keep `SRCREV` pinned. When I want new upstream changes, I **deliberately bump** it to the new commit hash and commit that change to my layer. My layer's Git history then records exactly which dashboard commit each image used.

> The pinned commit must be reachable from the branch named in `SRC_URI` (`branch=master`), or the fetcher will report that it can't find the revision.

---

## Part 8: Quick reference

| Variable / keyword | One-line meaning |
|---|---|
| `SRC_URI` | Where my sources come from |
| `SRCREV` | Which Git commit to build (pin it) |
| `AUTOREV` | "Latest commit on the branch", resolved at every parse |
| `S` | Directory of the unpacked source |
| `WORKDIR` | My recipe's private work folder |
| `D` | Fake root folder where `do_install` puts files |
| `DEPENDS` | Build these first and put them in my sysroot |
| `-native` | Built to run on the host PC |
| `recipe-sysroot` / `recipe-sysroot-native` | My private target / host sandboxes |
| `inherit` | Bring in a class with ready-made build logic |
| `PN` | Package name (usually the recipe name) |
| `FILES:${PN}` | Which installed files go into the main package |
| `RDEPENDS:${PN}` | Packages that must also be on the device at runtime |
| `RRECOMMENDS` | "Nice to have" runtime packages (off when `NO_RECOMMENDATIONS = "1"`) |
| `IMAGE_FSTYPES` | Final image formats (ext4, wic, ...) |
| sstate | Cache of task outputs, keyed by input hashes |

> **Version note:** In newer Yocto releases (5.1 "Styhead" and later), unpacked files go to `${UNPACKDIR}` instead of directly into `${WORKDIR}`. On those releases, use `${UNPACKDIR}/smarthome-dashboard.init` in `do_install`, and `S` for a Git source becomes `${UNPACKDIR}/git` (or follows the release's default). The recipe above uses the older layout.
