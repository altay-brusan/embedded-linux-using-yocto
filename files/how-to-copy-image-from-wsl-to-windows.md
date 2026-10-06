# How to Copy an Image from WSL to Windows and Flash It

I completely agree. It is much faster and completely avoids the headache of fighting Windows disk locks, PowerShell commands, and WSL pass-through configurations.

Using Raspberry Pi Imager also gives you a visual confirmation of the drive you are targeting, which removes the risk of accidentally overwriting the wrong disk with `dd`.

## Workflow

1. **Copy the decompressed image to your Windows C: or D: drive:**

   ```bash
   cp core-image-minimal-raspberrypi-cm3.rootfs.wic /mnt/c/Users/Public/
   ```

2. **Open Raspberry Pi Imager in Windows.**
3. Under **Operating System**, scroll down to the bottom, select **Use custom**, and browse to `C:\Users\Public\core-image-minimal-raspberrypi-cm3.rootfs.wic`.
4. Under **Storage**, select your 16GB drive.
5. Click **Next** and flash the drive.

Once it finishes, just safely eject the drive, plug it into your CM3 setup, and you are ready to boot.
