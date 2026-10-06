# Set Up the Yocto Build Environment with Python 3.11

Here is the clean, structured setup to get your Yocto build running again on your relocated WSL environment.

---

## Step 1: Install Required System Packages

Run these commands in your Ubuntu terminal to install all essential Yocto dependencies, modern compression tools (`lz4`, `zstd`), and Python 3.11:

```bash
# Update package repositories
sudo apt update && sudo apt upgrade -y

# Install standard Yocto build tools and system dependencies
sudo apt install -y gawk wget git diffstat unzip texinfo gcc build-essential \
chrpath socat cpio python3 python3-pip python3-pexpect xz-utils debianutils \
iputils-ping python3-git python3-jinja2 python3-subunit zstd lz4 file locales libacl1

# Install Python 3.11 (required for BitBake compatibility)
sudo add-apt-repository ppa:deadsnakes/ppa -y
sudo apt update
sudo apt install -y python3.11 python3.11-venv python3.11-dev
```

---

## Step 2: System & Environment Configurations

Set up your build locale, Git protocol rewrites, and the Python 3.11 isolated virtual environment:

```bash
# 1. Set locale
sudo locale-gen en_US.UTF-8
export LANG=en_US.UTF-8

# 2. Configure Git rewrites (forces HTTPS instead of blocked Git protocols)
git config --global url."https://git.yoctoproject.org/".insteadOf "git://git.yoctoproject.org/"
git config --global url."https://github.com/".insteadOf "git://github.com/"

# 3. Create Python 3.11 Virtual Environment
python3.11 -m venv ~/yocto-env
```

---

## Step 3: Clone Poky

Clone the Scarthgap LTS release of Yocto:

```bash
cd ~
git clone -b scarthgap https://git.yoctoproject.org/poky
```

---

## Step 4: Run the Build Process

Now execute the standard 3-step workflow to launch the build.

### 1. Activate the Python 3.11 environment and initialize Yocto

```bash
# Activate Python 3.11 environment
source ~/yocto-env/bin/activate

# Initialize Yocto build environment (moves you to ~/build)
cd ~/poky
source oe-init-build-env ../build
```

### 2. Configure threading limits (optional, recommended)

Before launching BitBake, limit thread concurrency in `~/build/conf/local.conf` to avoid WSL socket timeouts:

```bash
nano ~/build/conf/local.conf
```

Add or modify these lines:

```bitbake
BB_NUMBER_THREADS = "4"
PARALLEL_MAKE = "-j 4"
BB_HASHSERVE = "auto"
```

### 3. Trigger the image build

```bash
bitbake core-image-minimal
```
