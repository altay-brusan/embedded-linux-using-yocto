# Set Up the Yocto Build Environment with Python 3.11 (Bugfix)

The system package installation and trigger processing completed successfully.

The two error lines regarding systemd/dbus are benign in WSL:

```text
Failed to get properties: Transport endpoint is not connected
Failed to connect to system scope bus via local transport: Connection refused
```

They simply indicate that systemd service triggers tried to notify the system message bus during package updates, which happens when running inside WSL containerized environments.

You are now ready to continue with the next steps.

---

## Step 1: Create and Activate Python 3.11 Environment

Run this to set up the isolated Python 3.11 virtual environment in your home directory:

```bash
python3.11 -m venv ~/yocto-env
source ~/yocto-env/bin/activate
```

Verify that Python inside the environment shows 3.11:

```bash
python --version
```

---

## Step 2: Clone Poky (Scarthgap LTS)

Clone a fresh copy of Poky into your home directory:

```bash
cd ~
git clone -b scarthgap https://git.yoctoproject.org/poky
```

---

## Step 3: Initialize Build Environment

Initialize the build workspace (this automatically creates and moves you into `~/build`):

```bash
cd ~/poky
source oe-init-build-env ../build
```

---

## Step 4: Configure Threading Limits (Recommended for WSL2)

Open `~/build/conf/local.conf` to set conservative thread counts:

```bash
nano ~/build/conf/local.conf
```

Add or adjust these variables near the end of the file:

```bitbake
BB_NUMBER_THREADS = "4"
PARALLEL_MAKE = "-j 4"
BB_HASHSERVE = "auto"
```

Save and exit (`Ctrl + O`, `Enter`, `Ctrl + X`).

---

## Step 5: Start the Build

Trigger the compilation:

```bash
bitbake core-image-minimal
```
