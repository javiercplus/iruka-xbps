<p align="center">
  <img src="resources/icons/iruka.png" alt="iruka-xbps logo" width="128">
</p>

# iruka-xbps

A graphical front-end for the XBPS package manager, written in C++ using GTK 3.

## Features

- Local package searching and filtering.
- Visual package state representation (Installed, Not Installed, Outdated, Queued).
- Detail panel showing package descriptions, metadata, and files.
- Transaction queue to batch installation and removal operations.
- Polkit integration via pkexec for system repository synchronization and package modifications.
- Repository configuration editor.

## Prerequisites

- Void Linux (or any system running XBPS).
- C++17 compiler (GCC or Clang).
- GTK 3 development libraries.
- Polkit (for privilege escalation).
- Meson and Ninja build systems.

## Building and Installing

1. Configure the build directory:
   ```bash
   meson setup builddir --prefix=/usr
   ```

2. Compile the project:
   ```bash
   ninja -C builddir
   ```

3. Install the application:
   ```bash
   ninja -C builddir install
   ```

## Running

Run the binary directly from the terminal or using the desktop launcher:
```bash
iruka-xbps
```

## Configuration

Settings are saved in the user's config directory:
`~/.config/iruka-xbps/settings.conf`

## License

- GUI frontend code is licensed under the WTFPL (Do What the Fuck You Want to Public License).
- Core/backend integration code is licensed under the BSD 3-Clause License.
