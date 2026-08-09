# Contributing to iruka-xbps

Thank you for your interest in contributing to iruka-xbps. Please review the following guidelines before submitting code.

## Coding Standards

- Write standard C++17.
- Document all new functions, classes, and logic in clear English.
- Avoid using public classes for UI-internal details; keep implementation details scoped to private headers (e.g., `main_window_private.h`).
- Handle inputs and type conversions securely. For example, wrap string conversions in exception handlers (like `try/catch` around `std::stoi`).
- Ensure thread safety when bridging background operations back to the main GTK thread. Always use `g_idle_add` with proper validation checks (e.g., ensuring widgets have not been destroyed).
- Use modern C++ memory management (prefer smart pointers or container ownership rather than raw manual allocation where possible).

## Development Workflow

1. Fork the repository and create a feature branch.
2. Implement your changes.
3. Verify that your changes compile cleanly without warnings:
   ```bash
   ninja -C builddir
   ```
4. Submit a Pull Request detailing the changes, reasoning, and testing performed.

## Translations (i18n)

The UI is localized with gettext. Follow these rules when adding or modifying
translations:

1. **Wrap every user-visible string in `_()`** (e.g. `_("Synchronize package databases")`).
   Strings referenced through constants (e.g. `_(IRUKA_APP_TITLE)`) must also be
   repeated inside `_()` in `src/gui/i18n_strings.cpp`, otherwise xgettext cannot
   see the literal and it will never be translated.

2. **Keep the extraction list up to date** — make sure every file containing new
   translatable strings is listed in `po/POTFILES.in`.

3. **Regenerate the template and merge catalogs:**
   ```bash
   ninja -C builddir iruka-xbps-pot   # regenerate po/iruka-xbps.pot
   ninja -C builddir iruka-xbps-gmo   # recompile all .mo catalogs without relinking
   ninja -C builddir                  # full rebuild (also compiles the .mo catalogs)
   ```

4. **To add a brand-new language:**
   - Append the language code to `po/LINGUAS`.
   - Create `po/<lang>.po`, e.g. `msginit --input=po/iruka-xbps.pot --locale=<lang>`.
   - Fill in the translations and rebuild (see targets above).
   - **English is not listed in `po/LINGUAS`**: it is the source language (the
     `msgid` strings) and needs no catalog. The language menu entry for English
     uses the code `"en"`, which makes gettext fall back to the untranslated
     strings.

5. **Runtime notes:**
   - When running from the build directory the catalogs are located automatically
     via `<executable dir>/po`; otherwise set `IRUKA_LOCALEDIR` to the directory
     containing the `<lang>/LC_MESSAGES` folders.
   - The saved preference is the `ui_language` key in
     `~/.config/iruka-xbps/iruka-xbps.conf`; it always overrides the system locale.
   - The `Options > Language` menu applies changes immediately. When setting the
     initial checked state of the language radio items in `buildMenuBar()`, block
     their `activate` signals first — otherwise `setLanguage()` fires during UI
     construction and causes a rebuild loop.
