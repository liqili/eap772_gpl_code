# TP-Link GPL Code for WIFI7 AP Omada EAP772 and Festa F76

This GPL code is a merge set from **Omada EAP772** and **Festa F76** GPL code shared by TP-Link. The code can be successfully built in an **Ubuntu 16.04 Docker container**. It builds the kernel, rootfs, and produces a flashable image.

---

## Compile Steps

To build the project, use the following commands:

```bash
make BOARD_TYPE=ipq53xx_eap7xx_common PID=eap WEB_PAGES_ID=VI_2.0 eap_prep
make BOARD_TYPE=ipq53xx_eap7xx_common PID=eap WEB_PAGES_ID=VI_2.0 eap_develop
make BOARD_TYPE=ipq53xx_eap7xx_common PID=eap WEB_PAGES_ID=VI_2.0 eap_release
```
# TP-Link Device Configuration: Disabling Certificate Verification and Firmware Installation

This guide explains how to disable certificate verification via the device CLI and install OpenWrt firmware on TP-Link devices.

---

## OEM Easy Installation

Installing OpenWrt on this device is relatively straightforward. Follow these steps:

---

### Step 1: Set Up Network Connection
1. Connect to the device using the **PoE port**.
2. Configure a **static IP** on your computer:
   - Default fallback IP: `192.168.0.254`.
   - Note: The device runs a DHCP client and may acquire a different IP from your network's DHCP server. Use your router’s DHCP logs to confirm the IP if needed.

---

### Step 2: Enable SSH on the Device
1. Access the TP-Link web interface:
   - Open your browser and navigate to the device's IP address (e.g., `http://192.168.0.254`).
2. Go to **Management → SSH** and enable SSH in the web GUI.

---

### Step 3: Disable Certificate Verification
1. Connect to the device via SSH:
   - Use the following command:
     ```bash
     ssh admin@192.168.0.254
     ```
     Replace `192.168.0.254` with your device’s IP if it differs.
2. Run the following command to disable certificate verification:
   ```bash
   cliclientd stopcs

