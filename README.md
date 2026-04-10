# Network Switcher

This is an Aroma plugin to switch between network profiles on the Wii U.


## Usage

- Open the Wii U Plugin System Config Menu (press **L** + **🡓** + **SELECT**)

- Enter the **Network Switcher** settings.

- Select the network profile you want to use and press **A**.

Note: this change is temporary. The Wii U will always try to use the default profile when
it boots up.

In addition, you can also change:

  - The default Wii U network profile.

  - The vWii network profile.


## Build instructions

### Dependencies:

  - devkitPPC

  - wut

### Steps:

If you obtained the source from a repository clone with git, make sure to use the
`--recurse-submodules` option. You can also run the `external/update-submodules.sh` script
to manually update them.

If you obtained the source from a release tarball, you can skip step 0.

0. `./bootstrap`

1. `./configure --host=powerpc-eabi CXXFLAGS="-Os -ffunction-sections -fdata-sections -fipa-pta"`

2. `make`

3. (Optional) If the Wii U is named `wiiu` in the network:

       - `make run` if the [wiiload plugin](https://github.com/wiiu-env/ftpiiu_plugin) is
         enabled.

       - `make wiiu-install` or `make wiiu-uninstall` if the [ftpiiu
         plugin](https://github.com/wiiu-env/ftpiiu_plugin) is enabled.


## Building with Docker

If you have Docker set up, you can simply run the `docker-build.sh` script.
