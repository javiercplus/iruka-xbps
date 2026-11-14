<p align="center">
  <img src="resources/icons/iruka.png" alt="iruka-xbps logo" width="128">
</p>


# iruka-xbps
A graphical front-end for the XBPS package manager, written in C++ using FLTK.

<img width="871" height="680" alt="image" src="https://github.com/user-attachments/assets/81225ccf-73ad-4e37-8d95-4f589ab26673" />


## Features

- Local package searching and filtering with a debounced search entry.
- Category sidebar: All / Installed / Not installed / Outdated plus heuristic
  categories (Multimedia, Office, Programming, Education, Graphics, Network,
  Games, Security, System, Utilities, Other).
- Visual package state representation (Installed, Not Installed, Outdated, Queued).
- Detail panel showing package descriptions, metadata, files and size.
- Action queue to batch install, uninstall and unmark operations.
- A single privileged helper daemon (`iruka-helper`) launched once through
  pkexec/polkit; every privileged command is routed through it, so no repeated
  password prompts appear. The helper exits when Iruka closes.
- Repository configuration editor.
- Multilingual interface (English, Spanish, Russian, Japanese and Hungarian)
  with an in-app language switcher.

## Prerequisites

- Void Linux (or any system running XBPS).
- C++17 compiler (GCC or Clang).
- FLTK development libraries.
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
- The catalogs are installed under `/usr/share/locale/{es,ru,ja,hu}/LC_MESSAGES/`,
  which matches the `LOCALEDIR` compiled into the binary.
- Two binaries are installed: `iruka-xbps` (GUI) and `iruka-helper` (privileged
  daemon). They must live in the same directory, because the GUI locates the
  helper next to its own executable.
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
