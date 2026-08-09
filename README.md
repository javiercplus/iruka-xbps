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
- Multilingual interface (Spanish and Russian) with an in-app language switcher.

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

When running from `builddir` (without installing), the translation catalogs
are located automatically; the `Options > Language` menu works out of the box.

## Packaging with xbps-src

A Void Linux template lives in `pkg/iruka-xbps/template`. When building it,
keep in mind:

- `gettext` **must** stay in `hostmakedepends`: the Meson `i18n` module needs
  `msgfmt`/`xgettext`/`msgmerge`/`msginit` to compile the `.mo` catalogs, and
  xbps-src chroots only install what is listed there. Without it the package
  ships with no translations.
- The catalogs are installed under `/usr/share/locale/{es,ru}/LC_MESSAGES/`,
  which matches the `LOCALEDIR` compiled into the binary.
- The `distfiles`/`checksum` fields reference a GitHub release tarball. After
  changing the source, regenerate the tarball and update the `checksum` field
  (e.g. `xbps-src digest iruka-xbps`), otherwise the package is built from
  stale code.

## Running

Run the binary directly from the terminal or using the desktop launcher:
```bash
iruka-xbps
```

## Configuration

Settings are saved in the user's config directory:
`~/.config/iruka-xbps/iruka-xbps.conf` (includes the `ui_language` preference).

## License

- GUI frontend code is licensed under the WTFPL (Do What the Fuck You Want to Public License).
- Core/backend integration code is licensed under the BSD 3-Clause License.
