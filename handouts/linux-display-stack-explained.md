# The Linux Display Stack, from Basics to a Real Frame

A study note on how a Linux application (for example a Qt QML app with no desktop) gets pixels onto a screen. It goes from basic ideas, through each layer, to a step-by-step trace of one frame. A section near the end lists the points where my first mental model was wrong.

---

## 0. Abbreviations

| Short | Full form | One-line meaning |
|---|---|---|
| **X11** | X Window System, version 11 | The classic Linux display server (windows, input, drawing) |
| **UMS** | User Mode Setting | Old way: a user program (X) set the screen mode itself |
| **KMS** | Kernel Mode Setting | New way: the kernel sets the screen mode |
| **DRM** | Direct Rendering Manager | The kernel's graphics subsystem (contains KMS and GEM) |
| **GEM** | Graphics Execution Manager | The part of DRM that manages GPU memory and GPU jobs |
| **GBM** | Generic Buffer Management | Userspace library (in Mesa) that allocates buffers the GPU *and* display can both use |
| **libdrm** | Direct Rendering Manager library | Thin userspace C wrapper around DRM kernel calls |
| **ioctl** | Input/Output Control | The system call a program uses to send commands to a device driver |
| **GPU** | Graphics Processing Unit | The chip (or block) that renders/draws |
| **CPU** | Central Processing Unit | The main processor |
| **RAM** | Random Access Memory | Main memory |
| **VRAM** | Video Random Access Memory | Memory on a separate graphics card |
| **DDR** / **GDDR** | Double Data Rate / Graphics Double Data Rate | System memory type / graphics-card memory type |
| **UMA** | Unified Memory Architecture | CPU and GPU share the same RAM |
| **SoC** | System on Chip | CPU, GPU, display controller etc. on one chip (typical in embedded) |
| **PCIe** | Peripheral Component Interconnect Express | The bus a desktop graphics card plugs into |
| **DMA** | Direct Memory Access | Hardware copying memory without the CPU |
| **OpenGL** | Open Graphics Library | A drawing API |
| **GLES** / **OpenGL ES** | OpenGL for Embedded Systems | The smaller OpenGL used on embedded and mobile |
| **EGL** | Originally "Embedded-System Graphics Library"; Khronos now calls it the "Native Platform Graphics Interface" | Glue between a drawing API (GLES) and a place to draw (a surface) |
| **Mesa** | (a name, not an acronym) Mesa 3D | Open-source implementation of OpenGL/GLES, EGL, GBM, Vulkan + GPU drivers |
| **CRTC** | Cathode Ray Tube Controller (historical name) | The display-controller block that scans a buffer out at a given mode |
| **HDMI** | High-Definition Multimedia Interface | Video output connector |
| **VSync** | Vertical Synchronization | The moment the screen finishes one refresh and starts the next |
| **VT** | Virtual Terminal | The text consoles (Ctrl+Alt+F1...) |
| **fbdev** | Framebuffer Device | The old, simple Linux display interface (`/dev/fb0`) |
| **BO** | Buffer Object | A block of graphics memory managed by DRM/GBM |
| **API** | Application Programming Interface | A set of functions a library offers |
| **Qt** | (a name, pronounced "cute") | The application framework |
| **QML** | Qt Modeling Language | Qt's declarative UI language |
| **QPA** | Qt Platform Abstraction | Qt's plugin layer that hides the window system / display |
| **eglfs** | EGL Full Screen | A QPA plugin: one full-screen app, drawn with EGL, no desktop |

---

## 1. The big idea: two separate jobs

Putting an image on a screen is **two different jobs** done by **two different hardware blocks**:

```text
            JOB 1: DRAW                              JOB 2: SHOW
   "compute the colour of every pixel"     "send those pixels to the monitor,
                                             60 times a second, at 1920x1080"
                 │                                         │
                 ▼                                         ▼
         GPU 3D engine                           Display controller
     (runs shaders, fills memory)            (reads memory, drives HDMI)
                 │                                         ▲
                 └──────────►   a buffer in memory   ──────┘
                                (the "framebuffer")
```

Everything else in this note is software that organises these two jobs and the **shared buffer** between them.

- The **drawing** side: OpenGL ES, Mesa, the kernel's GEM.
- The **showing** side: KMS.
- The **buffer in the middle**: GBM allocates it; EGL hands it to OpenGL.

---

## 2. A little history: from X11 to KMS

### Before about 2008: User Mode Setting (UMS)

**X11** was a full display server: it managed windows, keyboard/mouse input and drawing. One of its many jobs was **mode setting**: telling the display hardware the resolution, refresh rate, pixel format and which output (HDMI, VGA...) is active.

It did this **from userspace**, poking the graphics registers directly as root. Problems:

- No graphics before X started (plain text boot).
- Flicker and glitches when switching between X and a text VT.
- If X crashed, the display could be left in a broken state.
- Each X driver had to re-implement hardware knowledge the kernel also needed.

### After about 2008: Kernel Mode Setting (KMS)

Mode setting moved **into the Linux kernel**, inside DRM. Hence the name **Kernel** Mode Setting.

```text
   BEFORE (UMS)                           AFTER (KMS)
 ┌────────────────┐                   ┌────────────────┐   ┌──────────────┐
 │  X server      │                   │  X / Wayland / │   │  eglfs app   │
 │  (sets modes   │                   │  any app       │   │  (no desktop)│
 │   itself, as   │                   └───────┬────────┘   └──────┬───────┘
 │   root)        │                           │  ask the kernel   │
 └───────┬────────┘                           ▼                   ▼
         │ pokes registers            ┌─────────────────────────────────┐
         ▼                            │  Linux kernel: DRM / KMS        │
   graphics hardware                  └───────────────┬─────────────────┘
                                                      ▼
                                              graphics hardware
```

**Why this matters for us:** once the kernel owns mode setting, *any* program can drive the display through a clean kernel interface. That is exactly what Qt's **eglfs** does: a single app drives the screen with no X server.

> **Correction to my first idea:** X11 was much more than "setting parameters"; mode setting was only one of its jobs. And X11 is not removed from Linux. It is being replaced on desktops by **Wayland**, which itself relies on KMS. KMS is **only for display hardware**; it is not a general kernel module for other devices.

---

## 3. The kernel side: DRM = KMS + GEM

**DRM (Direct Rendering Manager)** is the kernel's graphics subsystem. A GPU driver (e.g. `i915`, `amdgpu`, `vc4`, `panfrost`) plugs into it. User programs reach it through device files:

- `/dev/dri/card0`: the full device (display **and** rendering). Needed for KMS.
- `/dev/dri/renderD128`: a "render node", rendering only, no display control.

DRM has **two halves**:

```text
                    DRM  (Direct Rendering Manager)
        ┌─────────────────────────┬──────────────────────────┐
        │   KMS half              │   GEM / render half       │
        │   "the SHOW side"       │   "the DRAW side"         │
        │                         │                           │
        │ • resolution, refresh   │ • allocate GPU memory     │
        │ • which connector       │   (buffer objects, BOs)   │
        │ • which buffer to show  │ • submit GPU command      │
        │ • page flips on VSync   │   batches to the GPU      │
        └────────────┬────────────┴─────────────┬─────────────┘
                     ▼                          ▼
             display controller            GPU 3D engine
```

### The KMS pipeline, inside the "show" side

KMS describes the display path as a chain of objects. This is the model `libdrm` exposes:

```text
  Framebuffer ──► Plane ──► CRTC ──► Encoder ──► Connector ──► Monitor
  (a buffer      (a layer   (scans out  (converts   (the physical
   registered     on screen; at a mode:  to a signal  port: HDMI,
   with KMS)      can be     1920x1080   type)        DSI, DP...)
                  several:   @ 60 Hz)
                  primary,
                  cursor,
                  overlay)
```

KMS **does not change pixels**. But it must know **where the pixels are**, because it points the display controller at a buffer and says "scan this out on the next VSync". So KMS *does* care about the buffer's address and layout, just not its content.

---

## 4. What is a "framebuffer"?

A **framebuffer** is simply a block of memory that holds one complete screen image: a colour value for every pixel.

```text
 1920 x 1080 pixels, 4 bytes each (RGBA)  ≈ 8 MB of memory
 ┌──────────────────────────────────────┐
 │ px px px px px px px px px px px ... │  row 0
 │ px px px px px px px px px px px ... │  row 1
 │ ...                                   │
 └──────────────────────────────────────┘
```

The word is used in two ways in Linux:

| | Old: **fbdev** (`/dev/fb0`) | Modern: **DRM framebuffer** |
|---|---|---|
| What it is | One fixed memory area the CPU writes into | Any buffer object registered with KMS (`drmModeAddFB2()`) |
| GPU acceleration | No | Yes, the GPU renders into it |
| Several layers / double buffering / page flip | Very limited | Yes |
| Today | Legacy; often emulated on top of DRM for the boot console | What eglfs, Wayland and X use |

So in the modern stack, "framebuffer" = **a buffer the display controller is allowed to scan out**.

---

## 5. The userspace side: libdrm, Mesa, GLES, EGL, GBM

```text
 ┌────────────────────────────────────────────────────────────────┐
 │                  Application (Qt via eglfs)                     │
 └────────────────────────────────────────────────────────────────┘
          │ draw calls        │ setup / swap       │ buffer alloc
          ▼                   ▼                    ▼
 ┌────────────────────────────────────────────────────────────────┐
 │                        Mesa (userspace)                         │
 │   ┌────────────┐     ┌────────────┐     ┌────────────┐          │
 │   │ OpenGL ES  │     │    EGL     │     │    GBM     │          │
 │   │  (draw)    │◄───►│  (glue)    │◄───►│ (buffers)  │          │
 │   └────────────┘     └────────────┘     └────────────┘          │
 │        GPU-specific driver inside Mesa (vc4, v3d, iris,         │
 │        radeonsi, panfrost, etnaviv ...) turns GL into GPU code  │
 └────────────────────────────────────────────────────────────────┘
                              │
 ┌────────────────────────────────────────────────────────────────┐
 │              libdrm  (thin C wrapper over ioctls)               │
 └────────────────────────────────────────────────────────────────┘
                              │
 ═══════════ ioctl system calls on /dev/dri/card0 ════════════════
                              │
 ┌────────────────────────────────────────────────────────────────┐
 │         Linux kernel: DRM   =   KMS (show)  +  GEM (draw)        │
 └────────────────────────────────────────────────────────────────┘
                              │
               GPU 3D engine  +  display controller
```

### Each piece in one sentence

- **OpenGL ES**: the drawing language. "Use this shader, draw these triangles." It knows nothing about windows, screens or memory allocation.
- **Mesa's GPU driver**: translates OpenGL ES calls into the GPU's own machine instructions and submits them to the kernel (GEM).
- **GBM**: allocates buffers that **both** the GPU and the display controller can use. It lives in **userspace (Mesa)**, not in the kernel.
- **EGL**: connects OpenGL ES to a place to draw, and hands finished frames back.
- **libdrm**: the C functions Qt/Mesa call to talk to the kernel (`drmModeAddFB2`, `drmModePageFlip`, `drmModeSetCrtc` ...).

### What exactly does EGL do?

This was the hardest piece to place. OpenGL ES is deliberately **blind to the operating system**: it has no function for "open a window", "find a screen" or "use this buffer". GBM gives raw buffers but doesn't know about OpenGL. **EGL is the bridge.**

```text
     OpenGL ES                      EGL                        GBM
 "I can draw, but           "I'll connect you.            "I have memory,
  where? into what?"         Here is your canvas,          but I don't know
                             and when you finish           who will draw
                             I'll swap canvases."          into it."
```

EGL's concrete jobs:

1. **Open a display**: `eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, gbm_device)`, i.e. "we are drawing for a GBM/KMS screen".
2. **Choose a config**: `eglChooseConfig()`, i.e. colour depth, depth buffer, GLES 2 support.
3. **Create a context**: `eglCreateContext()` holds the OpenGL state (shaders, textures) for this thread.
4. **Create a surface on the GBM buffers**: `eglCreateWindowSurface(..., gbm_surface)`, the "canvas".
5. **Make current**: `eglMakeCurrent()`, i.e. "from now on, GL calls on this thread draw into that canvas".
6. **Swap**: `eglSwapBuffers()`, i.e. "this frame is finished; give me the next empty buffer".

**Analogy:** OpenGL ES is the painter, Mesa's driver is the painter's hand, GBM is the shop that makes canvases, **EGL is the easel**: it holds the right canvas in front of the painter and swaps in a fresh one when a painting is done.

**The trick of eglfs:** on a desktop, EGL binds GLES to an **X11 or Wayland window**. In eglfs, EGL binds the same GLES to a **GBM surface**. Same drawing code, different place to draw.

---

## 6. Memory: tiled vs linear, and GBM's real job

### Why GPU memory layout differs from what a screen reads

A display reads pixels **row by row**, left to right, top to bottom ("linear"). A GPU works on **small 2D blocks** (neighbouring pixels in a triangle), so it prefers memory where neighbours in 2D are also neighbours in RAM ("tiled").

Example: an 8 x 4 image, numbers show the order pixels sit in memory.

```text
 LINEAR (what a simple display reads)        TILED, 4x2 tiles (what a GPU likes)
  0  1  2  3  4  5  6  7                       0  1  2  3 │  8  9 10 11
  8  9 10 11 12 13 14 15                       4  5  6  7 │ 12 13 14 15
 16 17 18 19 20 21 22 23                      ───────────┼────────────
 24 25 26 27 28 29 30 31                      16 17 18 19 │ 24 25 26 27
                                              20 21 22 23 │ 28 29 30 31

 Pixel (x,y) is at  y*width + x            A whole 4x2 block is contiguous,
                                           so one cache line covers a small
                                           square of the picture.
```

If a display controller that only understands linear read the tiled buffer as if it were linear, the picture would come out **scrambled** in strips.

### So what does GBM actually do?

GBM's job is **choosing and allocating** a buffer whose format and layout **both sides can accept**. When Qt creates the buffers it passes flags like `GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING` ("the display must read this, the GPU must write it"). The driver then picks a layout (Linux calls this a **format modifier**):

```text
                     GBM asks the driver:
          "a buffer the GPU can render AND the display can scan out"
                                │
             ┌──────────────────┴───────────────────┐
             ▼                                      ▼
  Display controller CAN read           Display controller can ONLY read
  this GPU's tiled layout               linear
             │                                      │
             ▼                                      ▼
  allocate TILED buffer                  allocate LINEAR buffer
  (fast for GPU, display de-tiles        (GPU renders a little slower,
   in hardware while scanning out)        but no conversion needed)
```

Either way the buffer is created **in its final layout from the start**. There is **no "reorganise after drawing" step**, and **GBM does not move or copy data**. (A copy only happens in special cases, e.g. a separate conversion pass the driver adds, or a multi-GPU laptop. That is the exception, not the design.)

### Where the buffer lives: shared RAM vs graphics card VRAM

```text
 EMBEDDED / SoC (UMA)                          DESKTOP with a graphics card
 ┌─────────────── one chip ───────────────┐    ┌──── CPU side ────┐    ┌────────── graphics card ──────────┐
 │  CPU   GPU 3D engine  display ctrl     │    │  CPU             │    │  GPU 3D engine   display engine   │
 └───┬────────┬───────────────┬───────────┘    │                  │    │       │                │          │
     │        │               │                │  system RAM      │PCIe│       ▼                ▼          │
     ▼        ▼               ▼                │  (DDR)           │◄──►│   VRAM (GDDR)  ── buffers live    │
  ┌──────────────────────────────────┐         └──────────────────┘    │                   here            │
  │   one system RAM (LPDDR)         │                                 │           HDMI / DisplayPort ──►  │
  │   GBM buffer lives here; GPU     │                                 └───────────────────────────────────┘
  │   writes it, display reads it    │
  └──────────────────────────────────┘
```

- **Embedded (UMA):** CPU, GPU and display controller share **one** RAM. The GBM buffer is just a region of it. Zero copy: the GPU writes, the display reads, same bytes.
- **Desktop graphics card:** the card has its own **VRAM**, and the GBM buffer is allocated **in VRAM**. The GPU renders into VRAM, and the card's display engine scans out from VRAM. Still zero copy, just on the card.

### Why does a graphics card need so much of its own RAM?

Because rendering needs enormous **bandwidth** (textures, geometry, depth buffers, read over and over every frame), and fetching all of that across PCIe from system RAM would be far too slow.

| | System RAM (DDR5) | Graphics card VRAM (GDDR6/6X) |
|---|---|---|
| Optimised for | Low latency, unpredictable CPU access | Huge throughput, massively parallel access |
| Bus width | 64 to 128 bit | 256 to 384 bit |
| Typical bandwidth | ~50 to 100 GB/s | ~500 to 1,000 GB/s |

(Rough, order-of-magnitude figures.)

---

## 7. Two engines, even on one graphics card

My first assumption was: "the HDMI port is on the graphics card, so when the GPU finishes drawing it automatically sends the frame out." Half right: the port *is* on the card, but **drawing and showing are still two separate blocks**, and nothing is sent until **KMS tells the display engine** to switch buffers.

```text
 ┌──────────────────────── GPU chip / graphics card ─────────────────────────┐
 │                                                                            │
 │   ┌──────────────────────┐                     ┌──────────────────────┐    │
 │   │   3D ENGINE          │                     │  DISPLAY ENGINE       │    │
 │   │   runs shaders,      │                     │  (CRTC + planes)      │    │
 │   │   WRITES pixels      │                     │  READS pixels on a    │    │
 │   │                      │                     │  fixed clock, 60x/s   │    │
 │   └──────────┬───────────┘                     └──────────┬───────────┘    │
 │              │ controlled via GEM                         │ controlled via KMS
 └──────────────┼────────────────────────────────────────────┼───────────────┘
                ▼                                            ▼
        ┌──────────────────────────────────────────────────────────┐
        │   memory: Buffer A (being drawn)   Buffer B (on screen)   │
        └──────────────────────────────────────────────────────────┘
                                                             │
                                                        HDMI cable
```

The display engine **never stops**: every refresh it reads whichever buffer it is currently pointed at. The 3D engine works at its own pace. They meet only through the buffers and the page flip.

---

## 8. Double buffering, page flips and VSync

### The problem: tearing

If the GPU drew directly into the buffer the screen is currently reading, the top of the screen would show the old frame and the bottom the new one. That visible seam is called **tearing**.

### The solution: two (or three) buffers and a flip

```text
 time ──────────────────────────────────────────────────────────────►

 VSync:      |                 |                 |                 |
 Screen:     [ shows A ]       [ shows B ]       [ shows A ]
 GPU:          draws into B      draws into A      draws into B
                      │                 │
                      └─ flip at VSync ─┘─ flip at VSync ...

 "front buffer" = the one being shown     "back buffer" = the one being drawn
```

1. GPU draws the next frame into the **back** buffer.
2. App asks KMS to **page flip** (`drmModePageFlip()` or an atomic commit).
3. KMS waits for the next **VSync** (the gap between two screen refreshes) and switches the display engine's pointer to the new buffer. No pixels are copied, just a pointer change.
4. KMS sends a **"flip done" event**. The old front buffer is now free to draw into.

That is why an eglfs app is naturally limited to the refresh rate (e.g. 60 frames per second): it waits for the flip before reusing a buffer.

---

## 9. Where Qt fits: QPA and eglfs

**QPA (Qt Platform Abstraction)** is Qt's plugin layer for "how do I show windows on this system?". The same app code can run with different QPA plugins:

```text
                          Your Qt / QML application
                                     │
                       Qt Platform Abstraction (QPA)
     ┌──────────┬──────────┬─────────┼──────────┬────────────┬───────────┐
     ▼          ▼          ▼         ▼          ▼            ▼           ▼
   xcb      wayland    windows    cocoa      eglfs       linuxfb    offscreen
  (X11)    (Wayland)  (Windows)  (macOS)  (EGL full    (old fbdev,  (no screen)
                                            screen,     software
                                            no desktop) drawing)
```

Selected at runtime with `QT_QPA_PLATFORM=eglfs`.

**eglfs** has its own back-ends ("integrations"). On a standard Linux DRM system it uses **`eglfs_kms`**, which does everything in this note: opens `/dev/dri/card0`, uses KMS for the display, GBM for buffers, EGL + GLES for drawing. Selected with `QT_QPA_EGLFS_INTEGRATION=eglfs_kms` (often picked automatically).

### The full stack, with Qt on top

```text
  QML  (Main.qml)                       "a red rectangle here"
      │
  Qt Quick scene graph                  turns the UI tree into GL draw calls
      │
  QPA plugin: eglfs                     QT_QPA_PLATFORM=eglfs
      │
  eglfs_kms integration                 QT_QPA_EGLFS_INTEGRATION=eglfs_kms
      │
      ├── OpenGL ES 2  (draw)  ─┐
      ├── EGL          (glue)  ─┼── Mesa (userspace)
      └── GBM          (buffers)┘
      │
  libdrm
      │
  ═════ ioctls on /dev/dri/card0 ═════
      │
  DRM (kernel):   KMS (show)  +  GEM (draw)
      │
  GPU driver (kernel)  ──►  GPU 3D engine + display controller  ──►  HDMI ──► monitor
```

---

## 10. End-to-end example: one red frame

What actually happens when an eglfs app shows a red screen. Function names are the real ones; Qt does all of this for you internally.

### A. Start-up (once)

```text
 1. fd = open("/dev/dri/card0")                         ► talk to DRM
 2. drmModeGetResources / GetConnector                  ► KMS: which monitor is plugged in,
                                                           which modes (1920x1080@60) exist
 3. gbm_dev = gbm_create_device(fd)                      ► GBM on top of the same device
 4. gbm_surf = gbm_surface_create(gbm_dev, 1920, 1080,
        GBM_FORMAT_XRGB8888,
        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING)       ► a small pool of buffers that the GPU
                                                           can draw AND the display can show
 5. egl_dpy = eglGetPlatformDisplay(GBM, gbm_dev)        ► EGL: "we draw for a GBM screen"
    eglInitialize, eglChooseConfig, eglCreateContext
 6. egl_surf = eglCreateWindowSurface(egl_dpy, cfg,
        gbm_surf)                                        ► EGL wraps the GBM buffers as a canvas
 7. eglMakeCurrent(egl_dpy, egl_surf, egl_surf, ctx)     ► GL calls now draw into that canvas
```

### B. Every frame

```text
 8.  glClearColor(1, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT)
        │  Mesa driver turns this into GPU commands
        │  and submits them to the kernel (GEM)
        ▼
     GPU 3D engine fills the BACK buffer with red
 9.  eglSwapBuffers(egl_dpy, egl_surf)                    ► "frame finished" (EGL makes sure GPU
                                                             work is done/fenced, rotates buffers)
 10. bo = gbm_surface_lock_front_buffer(gbm_surf)         ► get the just-finished buffer object
 11. fb_id = drmModeAddFB2(fd, ..., handle of bo, ...)    ► register it with KMS as a framebuffer
                                                             (cached after the first time)
 12. first frame:  drmModeSetCrtc(fd, crtc, fb_id, ..., mode)    ► set 1920x1080@60, show this buffer
     later frames: drmModePageFlip(fd, crtc, fb_id, EVENT)        ► switch to it at next VSync
 13. wait for the "flip done" event (drmHandleEvent)
 14. gbm_surface_release_buffer(gbm_surf, previous_bo)     ► old front buffer is free to draw again
        │
        └──► back to step 8 for the next frame
```

Note: it is Qt's eglfs_kms code (steps 10 to 14) that calls the page flip, not `eglSwapBuffers` itself. Newer Qt versions may use "atomic" KMS (`drmModeAtomicCommit`) instead of `drmModePageFlip`; the idea is the same.

### The same frame as a picture

```text
   App (Qt)          Mesa (GLES/EGL/GBM)        Kernel DRM            Hardware
      │                     │                       │                     │
      │ glClear(red) ──────►│ build GPU commands ──►│ GEM: submit job ───►│ 3D engine
      │                     │                       │                     │ writes red
      │                     │                       │◄── job done ────────│ into buffer B
      │ eglSwapBuffers ────►│ fence, rotate bufs    │                     │
      │ lock_front_buffer ─►│ returns BO "B"        │                     │
      │ drmModePageFlip(B) ─┼──────────────────────►│ KMS: at next VSync  │
      │                     │                       │ point CRTC at B ───►│ display engine
      │                     │                       │                     │ reads B ──► HDMI
      │◄── flip-done event ─┼───────────────────────│                     │
      │ release BO "A" ────►│ A is free again       │                     │
```

---

## 11. Where my first mental model was off

| What I thought | How it actually works |
|---|---|
| X11 only set display parameters | X11 was a full display server; mode setting was one of its jobs. It's being replaced by Wayland, not removed from Linux. |
| KMS is a general kernel module, not only graphics | KMS is only for display hardware, and it's one half of DRM. |
| DRM was introduced to convert formats | DRM is the kernel graphics subsystem: KMS (show) + GEM (draw/memory). Format choice happens at allocation, not as a conversion step. |
| GBM is a buffer inside DRM | GBM is a **userspace** library in Mesa. It asks the kernel (via GEM) to allocate buffers. |
| The "writer" side is called FEM | It's **GEM** (Graphics Execution Manager). |
| KMS doesn't care about buffers at all | KMS doesn't touch pixels, but it must know which buffer to scan out and its layout. |
| EGL translates OpenGL commands into GPU format | Mesa's GPU driver translates GL into GPU instructions. EGL only connects GL to a surface and swaps buffers. |
| EGL allocates the blank frame | GBM allocates; EGL wraps those buffers as the drawing surface. |
| GPU draws, then something re-tiles the pixels | The buffer is created in its final layout (tiled or linear, whatever both sides accept). No re-sorting pass. |
| GBM moves frames from main RAM to GPU RAM | GBM doesn't copy. On SoCs everything is in shared RAM; on a graphics card the buffer is allocated directly in VRAM. |
| When the GPU finishes, it automatically sends the frame to HDMI | The display engine is separate. The app asks KMS for a page flip, and KMS switches the display engine's pointer at VSync. |
| Qt talks to OpenGL only through eglfs | Qt Quick issues GL calls itself. eglfs (a QPA plugin) provides the *window/screen* those calls draw into. |

---

## 12. Tie-in to the Yocto build

To make this chain exist on the target, Qt and Mesa must be built with each link enabled:

```bitbake
PACKAGECONFIG:append:pn-qtbase = " eglfs gles2 kms gbm"
```

| Flag | Link in the chain | Without it |
|---|---|---|
| `eglfs` | The QPA plugin (full-screen, no desktop) | No eglfs plugin to load |
| `gles2` | OpenGL ES 2 as the drawing API | Nothing to draw with |
| `kms` | The eglfs_kms back-end (talks to DRM/KMS via libdrm) | Plugin exists but can't reach the display |
| `gbm` | Buffer allocation shared by GPU and display | No way to get scan-out buffers |

Plus, at runtime:

```bitbake
RDEPENDS:${PN} += "qtbase-plugins qtdeclarative-qmlplugins mesa-megadriver"
```

`mesa-megadriver` is the Yocto package containing Mesa's hardware drivers. Without it, EGL/GLES/GBM have no GPU driver underneath.

And the environment the init script typically sets:

```sh
export QT_QPA_PLATFORM=eglfs
export QT_QPA_EGLFS_INTEGRATION=eglfs_kms   # usually auto-detected
```

---

## 13. One-page summary

```text
 DRAW side                      SHARED BUFFER                     SHOW side
 ─────────                      ─────────────                     ─────────
 QML / Qt Quick                                                   
   │ GL calls                                                     
 OpenGL ES  (what to draw)                                        
   │                                                              
 Mesa driver (GL → GPU code)    GBM allocates buffers that        KMS (in kernel DRM):
   │                            BOTH sides accept; EGL makes      mode, connector,
 GEM in kernel (submit jobs)    them the GL "canvas" and           which buffer to show,
   │                            swaps them each frame             page flip at VSync
 GPU 3D engine  ───writes───►   [ buffer A ] [ buffer B ]  ───reads───►  display engine ──► HDMI
```

- **KMS** = show. **GEM** = draw/memory. Together = **DRM** (kernel).
- **GBM** = buffers both sides accept (userspace, Mesa).
- **EGL** = glue between GLES and those buffers.
- **libdrm** = C wrapper for kernel calls.
- **Framebuffer** = a buffer the display is allowed to show.
- **Page flip at VSync** = how a new frame appears without tearing.
- **eglfs** = Qt's QPA plugin that runs all of this with no desktop.
