# D-Robotics xbuild system user guide

## Development environment

### Host environment
Supports Host environment compilation on 18.04, 20.04 and 22.04, Ubuntu 20.04 is recommended

### Install Prerequisites

	sudo build/install_host_deps.sh

### Install python package
ubuntu 18.04 systems need to install the python package

	pip3 install cryptography

ubuntu 20.04 and 22.04 systems need to install the python package

	pip3 install cryptography==40.0.2

### Cross-Compile Toolchain Setup Guide
The SDK utilizes the Cross-Compile Toolchain version arm-gnu-toolchain-11.3.rel1, stored in the SDK's toolchain directory as a compressed archive named arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz.

Before build the SDK, it's necessary to extract the Cross-Compile Toolchain to the directory specified in the board configuration file (e.g., device/horizon/x5/board_xxx_config.mk). These configuration files are typically stored in the device/horizon/x5 directory, offering multiple configurations to choose from. Select an appropriate configuration based on your requirements.

Taking the example of the device/horizon/x5/board_x5_soc_debug_config.mk board configuration, the default path for the toolchain is in /opt/:

	export TOOLCHAIN_PATH=/opt/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-linux-gnu

To install the toolchain into the specified directory (e.g., /opt/), use the following command. Note that writing to the /opt/ directory typically requires sudo privileges. If sudo access is not convenient, install the toolchain in another directory and update the TOOLCHAIN_PATH option accordingly in the board configuration.

For installation with sudo privileges:

	sudo tar -xvf toolchain/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz -C /opt/

Or, for installation without sudo privileges:

	tar -xvf toolchain/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz -C /home/hobot/

Ensure to modify the TOOLCHAIN_PATH option in the board configuration file based on your preferences and access permissions:

	export TOOLCHAIN_PATH=/home/hobot/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-linux-gnu

This step allows flexibility in choosing the installation directory based on your preferences and access permissions.

## SDK Source Code
SDK directory structure is as follows:
```text
.
├── bd.sh -> build/xbuild.sh    # Linked to the main compilation program, users can directly execute this file to initiate compilation
├── device                      # Board configuration directory, each hardware type corresponds to a configuration file, allowing for setting compilation options and partition tables, etc.
├── build                       # Code directory for the compilation system, providing shell scripts for compiling various functional modules and tools used in compilation
├── miniboot                    # Generates the minimal boot firmware containing gpt, mbr, bl2, ddr, bl3x
├── uboot                       # U-Boot source code
├── hobot-drivers               # Kernel driver source code, containing Horizon-developed driver code
├── kernel                      # Original Linux kernel source code
├── system                      # Includes various types of root file systems, such as initramfs and root file systems generated through buildroot
├── hbre                        # Multimedia library source code
├── app                         # Application and test program source code, providing system stress testing programs, example code for functional modules, etc.
├── adsp                        # DSP-related source code
├── toolchain                   # Cross-compilation toolchain
├── prebuilts                   # Provides precompiled modules, such as closed-source parts like libbpu library and bpu-hw_io driver
├── README.md -> build/README.md
└── out                         # Compilation output directory
```

### Build output directory
``` text
out
├── build                           # External compilation output directory for source codes such as uboot, kernel, hbre, etc.
│   ├── hbre_deps
│   ├── hbre
│   ├── hobot-drivers
│   ├── kernel
│   ├── test
│   └── uboot
├── build_log                       # Directory for saving compilation log files
│   └── build_20240204_134224.log
├── deploy
│   ├── app
│   ├── boot
│   ├── hbre
│   ├── miniboot
│   ├── system
│   ├── uboot
│   └── vbmeta
└── product
    ├── app.img
    ├── boot.img
    ├── hbre.img
    ├── emmc_disk.img               # Complete image packed after compilation
    ├── emmc_disk.simg              # Sparse format complete image
    ├── miniboot.img
    ├── board_config.mk             # Board configuration file for user reference
    ├── system.img
    ├── uart_usb                    # Firmware required by flashing tools, generated separately by the pack command or ./bd.sh factory
    ├── ubootenv.img
    ├── uboot.img
    ├── vbmeta.img
    ├── veeprom.img
    └── x5-soc-debug-gpt.json       # Partition table containing complete fields after being expanded by partition analysis tools

```

## Build
### Host full command
``` text
==================================================================================
   \  //   Welcome to the D-Robotics xbuild system!
    \//    Working directory: <workspace>
    //\
   //  \
==================================================================================
Available commands for xbuild.sh:
./xbuild.sh [all | function] [module] [clean | distclean]
Support functions :
	help lunch miniboot uboot factory boot hbre system app pack
Usage example:
	./xbuild.sh all
	./xbuild.sh miniboot [clean | distclean]
	./xbuild.sh uboot [clean | distclean]
	./xbuild.sh boot [clean | distclean]
	./xbuild.sh boot module [clean]  -- eg: ./xbuild.sh boot spi
	./xbuild.sh system [clean | distclean]
	./xbuild.sh hbre [help | all ] [modules] [pack | clean | distclean]
	./xbuild.sh hbre module [pack | clean | distclean] -- eg: ./xbuild.sh hbre liblog
	./xbuild.sh app [help | all ] [modules] [pack | clean | distclean]
	./xbuild.sh app module [pack | clean | distclean]
	./xbuild.sh clean
	./xbuild.sh distclean
	./xbuild.sh uboot menuconfig
	./xbuild.sh boot menuconfig
	./xbuild.sh help
==================================================================================
```

### Host shortcut command
``` text
After executing 'source build/quickcmd.sh'
The following Shortcut commands can be used:
Available commands for bd.sh:
	help all clean distclean lunch miniboot uboot factory boot system hbre app pack

Shortcut commands for build:
	b      : bd.sh             - default build all
	ball   : bd.sh all         - build all
	bm     : bd.sh miniboot    - only build miniboot
	bu     : bd.sh uboot       - only build uboot
	bf     : bd.sh factory     - build uart_usb image
	bb     : bd.sh boot        - only build kernel
	bs     : bd.sh system      - only build rootfs
	bh     : bd.sh hbre        - build hbre
	bhm    : bd.sh hbre module - build hbre module, user interactive mode
	ba     : bd.sh app         - build app
	bam    : bd.sh app module  - build app module, user interactive mode
	bp     : bd.sh pack        - pack all image into emmc_disk.img

Shortcut commands for changing directory:
	croot  - go to root directory
	cr     - go to root directory
	cout   - go to out directory
	co     - go to out directory
	cuboot - go to uboot directory
	cub    - go to uboot directory
	cboot  - go to kernel directory
	cb     - go to kernel directory
	cdev   - go to device directory
	cbuild - go to device directory
	capp   - go to app directory
	chbre  - go to hbre directory
	go <regex> -- go to directory matching the specified <regex>

Shortcut commands for configuring U-Boot and the kernel defconfig
	bumc   : bd.sh uboot menuconfig       - Edit and save uboot menuconfig
	bbmc   : bd.sh boot menuconfig        - Edit and save kernel menuconfig

Usage example for build kernel and hbre module
	bb spi      : bd.sh boot spi          - Compile driver under kernel
	bh liblog   : bd.sh hbre liblog       - Compile modules under hbre
==================================================================================
```

### build all

To perform a complete compilation without any parameters，execute ./bd.sh：

	./bd.sh

Upon successful compilation, all image files will be generated in the output directory (out).

### build module

The bd.sh script supports modular compilation by specifying different functions and modules. The generated image files will be output to the compilation image directory (out). Use the following syntax:

``` bash
./bd.sh [all | function] [module] [clean | distclean]
Supported functions:
	help lunch miniboot uboot factory boot hbre system app pack
```
Functions like miniboot, uboot, boot, hbre, system, and app correspond to generating images. During compilation, all build logs are output for easy reference when individually compiling a module. Each partition with actual content has a corresponding function option for image compilation. Users can debug a specific module, compile it separately, and update it to the board after compilation. All modular compilation functions support clean and distclean commands, for example:

``` bash
# Only build uboot
./bd.sh uboot

# Clean uboot
./bd.sh uboot clean

# Distclean uboot
./bd.sh uboot distclean
```

After compiling all modules or updating a few specific modules and wanting to generate the complete image, execute the pack command to repack emmc_disk.img:

``` bash
./bd.sh pack
```

hbre and app compilation also support finer-grained compilation, making it convenient to debug smaller module functionalities. For instance, to individually compile the liblog in the hbre directory:

``` bash
# Only build liblog
./bd.sh hbre liblog

# build liglog and pack hbre.img
./bd.sh hbre liblog pack

# If you don’t remember the module name, you can execute the following command to enter user interaction mode and select the module by entering numbers.
./bd.sh hbre module
```

The factory command generates firmware required for flashing tools, stored in out/product/uart_usb. It requires prior compilation of miniboot and uboot modules. The flashing tool downloads the firmware to the board's memory via serial port or usb interface, runs it in uboot, and then supports burning the complete image to the board's emmc or nand flash via USB.

``` bash
./bd.sh factory
```

Note: Users can also directly enter the build directory and execute corresponding compilation scripts (./mk_*.sh) for specific modules. For example:

``` bash
cd build

./mk_boot.sh
./mk_boot.sh clean

./mk_uboot.sh
./mk_uboot.sh clean
./mk_uboot.sh distclean

./mk_hbre.sh
./mk_hbre.sh liblog
./mk_hbre.sh camsys/libvpf
```

### clean

Deletes generated images and intermediate files without clearing project configuration files. Supports both complete project clean and individual module clean.

``` bash
# Complete project clean
./bd.sh clean

# Individual module clean
./bd.sh boot clean
```

### distclean

Deletes generated images and intermediate files, clearing all configuration files, including board-level configuration links, .config files for uboot and kernel, etc. Supports both complete project distclean and individual module distclean.

```bash
# Complete project distclean, attempts to restore the SDK to its state before compilation
./bd.sh distclean

# Individual module distclean
./bd.sh system distclean
```

### help
The help function can output all supported buili commands and help information.

``` bash
$ ./bd.sh help
Available commands for bd.sh:
./bd.sh [all | function] [module] [clean | distclean]
Support functions :
        help lunch miniboot uboot factory boot hbre system app pack
   ... ...
```

### Other build Functions

#### Set up and save kernel configuration
``` bash
./bd.sh boot menuconfig
# or shortcut command
bbmc
```

#### Set up and save uboot configuration
```bash
./bd.sh uboot menuconfig
# or shortcut command
bumc
```

#### Build kernel modules separately

```bash
./bd.sh boot <kmod_name>
# or shortcut command
bb <kmod_name>
```
Compiling the driver in the kernel/drivers directory.

For example:
1. compile ko under kernel/drivers/spi, where "<kmod_name>" is "spi"
```bash
./bd.sh boot spi
# or shortcut command
bb spi
```
