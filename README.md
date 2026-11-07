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

### Device identity

A built image identifies as a **Festa F76** (`Festa F76(TP-Link|UN|BE11000-T):1.0`) but runs Omada (EAP) features,
as the F65 tree's build does.
- This firmware takes the board only from the image's `fw_data/product-info`: the model the web UI shows, the
  hardware ID and the board configuration it runs (`/etc/<board>`). The build installs the `productinfo.bin` of the
  board named by `GPL_PRODUCT_BOARD` (in `build/qca/Makefile`), `F76_1_0_0` by default, so the image runs
  `/etc/Festa_F76_1.0`. The signature keys in it are the same for all three boards. For an image that identifies as an
  EAP772 instead, build with `GPL_PRODUCT_BOARD=EAP772_2_0` (or `EAP723_1_0`). The multi-board default,
  `mkimage/common/productinfo.bin.default`, stays EAP772, and the tree's prebuilt rootfs keeps that identity: only a
  build applies `GPL_PRODUCT_BOARD`.
- Behaviour stays Omada. The web UI would switch to the Festa menu and setup flow on a `supportFesta` flag, but this
  firmware's `data/sysmod.json` never sends one, whatever the identity. `/etc/Festa_F76_1.0/dev_config.json` sets
  `"festaFeatures": false` (stock F76 has `true`) as the F65 build does, although nothing in this firmware reads that
  flag.

The upgrade check only accepts an image whose support-list contains the device's current identity. This image's
lists Festa F76, EAP772 and EAP723, so it installs over stock F76 or over an earlier build that identified as EAP772.
Stock F76 firmware lists only Festa F76, so it can be installed back over a build that identifies as Festa F76.

## Simulating the firmware in Docker

`sim/` runs a firmware rootfs's userland in an arm64 container under qemu-user, so you can try the web UI and
`uclited` without flashing a device. The firmware's kernel, the flash, TP-Link's kernel modules and the Wi-Fi, BLE and
switch hardware are all missing. The simulation covers the web UI, the login and the config daemons, not the radios
or Omada adoption.

Docker needs to run arm64 containers. Docker Desktop can by default; colima can with `binfmt: true` in `colima.yaml`
(the default). On Linux, install the emulation with `docker run --privileged --rm tonistiigi/binfmt --install arm64`.
`python3` must be on the host.

1. Make the simulation image:
   ```bash
   sim/make-image.sh
   ```
   With no argument this imports the squashfs this tree last built
   (`images/eap/ipq53xx_eap7xx_common/ipq53xx_eap7xx_common-squashfs-root.img`). You can also pass another squashfs
   image (for example the rootfs volume of a flash dump) or a rootfs directory, such as the tree's prebuilt one:
   ```bash
   sim/make-image.sh eap_gpl_code_eap772/images/eap/ipq53xx_eap7xx_common/rootfs
   ```
   The rootfs is converted with `sim/sqfs2tar.py` (no `unsquashfs` needed) and imported as `eap772-fw-sim:latest`. The
   first run also builds the build image and cross-compiles `sim/nlshim.so` with the firmware toolchain, unpacking it
   inside a throwaway container if `eap_prep` has not. Run it again after each firmware build.
2. Start the simulation with the `sim` service in `docker-compose.yml` (`docker compose` works the same):
   ```bash
   docker-compose up -d sim
   docker-compose logs -f sim
   ```
   It is ready when the log shows `[sim] init done`. Under emulation that takes a while: `uclited` alone took about
   12 minutes in testing. Errors about `/proc/tplink_gpio`, `/dev/ttyMSM1`, iptables and `arping` are expected.
3. Open https://localhost:9443 and log in as `admin` / `admin`. The certificate is self-signed. `sim/login-test.sh`
   does the same login with curl. The ports differ from the F65 simulation's (8080/8443), so both can run at once.

Two variables choose what the simulated flash holds:
- `SIM_BOARD`: `F76_1_0_0` (default), `EAP772_2_0` or `EAP723_1_0`. This sets the unit's factory `device-info` and the
  starting config. It does not choose the board configuration the firmware runs: as on a real unit, `nvrammanager`
  takes that from the image's own `/fw_data/product-info` (see [Device identity](#device-identity)). So an image
  built from this tree comes up as a Festa F76 (`/etc/Festa_F76_1.0`) whatever `SIM_BOARD` is, and the tree's prebuilt
  rootfs as an EAP772 (`/etc/EAP772_2.0`).
- `SIM_CONFIG`: `factory` (default) starts from that board's `mkimage/<board>/default-config.bin`, as a new unit is
  flashed; `none` starts without a user config, as after a reset.

For example `SIM_BOARD=EAP772_2_0 SIM_CONFIG=none docker-compose up -d --force-recreate sim`.

Settings changed in the web UI are kept while the container exists. To stop, run `docker-compose stop sim`. To start
again from the factory defaults, run `docker-compose up -d --force-recreate sim`.

How it works:
- `sim-init.sh` is the container's command, a cut-down `rcS` and `rc.eapcs`. In place of `mount_tpdata` it writes the
  factory data `nvrammanager` reads to plain files under `/tp_data`: the board's GPL `device-info.bin`, a US region,
  additional hardware version 0 and a made-up MAC `02:77:72:76:00:01`. It then starts the daemons and `uclited`, and
  `httpd_portal` once `uclited` has created `/tmp/ucliteok`.
- `nlshim.c` is preloaded into the daemons. TP-Link's kernel modules provide netlink families that don't exist here, so
  the shim replaces those sockets with sockets that accept and discard everything. It also sends `uclited`'s hostname
  write to a file, because `/proc/sys` is read-only in a container. It is the same shim as the F65 tree's.
- `start-deps.sh` starts `wscheduler`, whose `/tmp/wscheduler_ok` marker `uclited` waits for, and creates the marker if
  the daemon doesn't within 20 s. `uclited-loop.sh` restarts `uclited` if it exits.

Helpers:

| Command | What it does |
|---|---|
| `docker exec eap772-sim sh /sim/uclited-ctl.sh status\|stop\|start\|restart` | show, stop or (re)start `uclited` and its restart loop |
| `sim/login-test.sh [password]` | log in with curl and fetch the main page; output in `logs/sim/` |

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

