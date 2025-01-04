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
./build/minimalist-Lockscreen --image /path/to/image.png --suspend 600
```

- `--image` is the path to the image you want to use as a wallpaper.
- `--suspend` is the time in seconds after which the computer will be suspended (`systemctl suspend` is called).

Alternatively, you can use the `--color` argument to specify a solid background color:

```bash
./build/minimalist-Lockscreen --color "ff0000" --suspend 600
```

- `--color` sets the background color as a hexadecimal value. RGB (ex: `#ff0000`) and RGBA (ex: `#ff0000ff`) values are supported.

If both `--image` and `--color` are provided, `--image` takes precedence.

## Controlling the lockscreen

The application listens to DPMS and Screensaver events to lock the screen when the screen is turned off and the screensaver is activated (if screensaver is enabled after the screensaver timeout).

### Controlling screen power

```sh
xset dpms 300 360 420 # Standby Suspend Off
```

### Screensaver timeout

```sh
xset s on
xset s 330 0
```

## Disable screen locking and screen saver

```bash
xset s off
xset -dpms
```

## You can check your current settings with

```sh
xset q
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
