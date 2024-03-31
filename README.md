# Minimalist Lockscreen [![GitHub Workflow Status (with event)](https://img.shields.io/github/actions/workflow/status/flipflop133/minimalist-lockscreen/makefile.yml)](https://github.com/flipflop133/minimalist-lockscreen/actions)
![preview](https://github.com/flipflop133/minimalist-lockscreen/assets/48946818/c769b087-acac-4729-bb36-7a4d9008b677)
[Image source](https://unsplash.com/photos/person-sitting-inside-restaurant-zlABb6Gke24)
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
- [ ] make date and unlock indicator modules as well

## Stuff to add

- [ ] support for config file
- [ ] support for custom colors
- [ ] support for multiple wallpapers for multiple monitors
- [ ] custom modules support (ex: display weather, music, calendar at given positions)
- [ ] video support
