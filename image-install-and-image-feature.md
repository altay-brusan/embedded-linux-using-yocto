# `IMAGE_INSTALL` and `IMAGE_FEATURES`

To understand **`IMAGE_FEATURES`**, think of them as **high-level toggles or "presets"** that configure the operational capabilities of your Linux image without requiring you to manually list every underlying package, config file, or user account setting.

While an **`IMAGE_INSTALL` package** brings in a specific software application (like `htop` or `nginx`), a **Package Feature (`IMAGE_FEATURES`)** configures a broader system behavior, often combining multiple packages, system policies, user accounts, and build flags into one keyword.

---

## 1. `IMAGE_INSTALL` vs. `IMAGE_FEATURES`

| Concept | What It Is | Example | What It Actually Does Behind the Scenes |
| --- | --- | --- | --- |
| **`IMAGE_INSTALL`** | Direct list of software packages (`.rpm` / `.deb`) to copy into the rootfs. | `IMAGE_INSTALL += "i2c-tools"` | Downloads, compiles, and installs the `i2c-tools` package binaries into `/usr/bin/`. |
| **`IMAGE_FEATURES`** | High-level system capability toggle / build flag. | `IMAGE_FEATURES += "ssh-server-dropbear"` | 1. Installs `dropbear` package.<br>2. Configures init/systemd service files.<br>3. Generates host SSH keys at boot.<br>4. Sets up firewall/port rules if enabled. |

---

## 2. Common Categories of `IMAGE_FEATURES`

Yocto groups `IMAGE_FEATURES` into logical capabilities that every embedded system typically needs to tune:

### A. Security & Access Control

- **`debug-tweaks`**: Allows root logins without a password (useful during development, **must be removed** for production).
- **`read-only-rootfs`**: Configures the root filesystem as read-only and sets up RAM-backed `/var` and `/tmp` (`tmpfs`) so flash memory isn't corrupted by power cuts.
- **`allow-empty-password`**: Permits empty passwords for existing accounts.

### B. Remote Management & Connectivity

- **`ssh-server-dropbear`**: Pulls in Dropbear (lightweight SSH server) and sets up boot scripts.
- **`ssh-server-openssh`**: Pulls in full OpenSSH for systems needing SFTP or advanced key features.

### C. Development & Debugging Tools

- **`tools-debug`**: Installs debugging utilities like `gdbserver`, `strace`, and `pstack`.
- **`tools-profile`**: Installs profiling tools like `perf`, `valgrind`, and `systemtap`.
- **`package-management`**: Keeps `dnf`/`opkg` inside the final image so you can run `dnf install` on the live board.

### D. User Experience & Graphics

- **`splash`**: Enables splash screen animation during kernel boot (`psplash`).
- **`x11-base`** / **`wayland`**: Pulls in window managers, display servers, and input driver stack.

---

## 3. Clarifying Kernel vs. User Space in Images

To clear up your earlier intuition: **`core-image` includes both Kernel and User Space elements**, but `IMAGE_FEATURES` controls how user-space services are assembled and configured.

```text
+-------------------------------------------------------------------------------+
| IMAGE_FEATURES                                                                |
| Toggles presets (e.g., "ssh-server-dropbear", "read-only-rootfs", "splash")   |
+-------------------------------------------------------------------------------+
       │                                     │
       ▼                                     ▼
+-----------------------------+       +-----------------------------------------+
| IMAGE_INSTALL               |       | SYSTEM CONFIGURATION                    |
| Individual packages         |       | Generates ssh keys, adjusts fstab,      |
| (dropbear, psplash, etc.)   |       | modifies pam/shadow password files      |
+-----------------------------+       +-----------------------------------------+
       │                                     │
       └──────────────────────┬──────────────┘
                              ▼
                     [ Final RootFS Generation ]
```

- **Kernel Space** stays in the kernel image (`zImage` / `Image`) and device tree (`.dtb`).
- **User Space** is assembled by Yocto using **`IMAGE_INSTALL`** (what programs exist) and **`IMAGE_FEATURES`** (how those programs and system rules are configured together).

---

## 4. How to Use `IMAGE_FEATURES` in Your Custom Image

Inside your `my-custom-image.bb` recipe:

```bitbake
SUMMARY = "Custom CM3 Production Image"
LICENSE = "MIT"

inherit core-image

# Enable SSH access and splash screen via features
IMAGE_FEATURES += " \
    ssh-server-dropbear \
    splash \
"

# For development, you can add debug-tweaks:
# EXTRA_IMAGE_FEATURES += "debug-tweaks"

# Add custom application packages
IMAGE_INSTALL += " \
    example \
    i2c-tools \
"
```
