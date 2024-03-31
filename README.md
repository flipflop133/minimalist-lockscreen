# Minimalist Lockscreen [![GitHub Workflow Status (with event)](https://img.shields.io/github/actions/workflow/status/flipflop133/minimalist-lockscreen/makefile.yml)](https://github.com/flipflop133/minimalist-lockscreen/actions)

## Building

### Install dependencies

#### Ubuntu

```bash
sudo apt-get update
```

```bash
sudo apt-get install -y libx11-dev libxfixes-dev libxrandr-dev xserver-xorg-dev libxinerama-dev libpam0g-dev libxft-dev
```

### Build the project

Building release build:

```bash
./build.sh
```

Building debug build:

```bash
./build.sh debug
```

Cleaning:

```bash
./build.sh clean
```

## Running

```bash
./build/minimalist-Lockscreen --image /path/to/image.png
```

## TODO (sorted by priority)

- [ ] only redraw the part of the wallpaper that needs to be redrawn
- [ ] allow missing args

## Stuff to add

- [ ] support for config file
- [ ] support for custom colors
- [ ] support for multiple wallpapers for multiple monitors
- [ ] custom modules support (ex: display weather, music, calendar at given positions)
