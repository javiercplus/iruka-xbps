project('iruka-xbps', 'cpp',
  version: '0.1.0',
  license: ['WTFPL', 'BSD-3-Clause'],
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
  ],
)

# FLTK can be discovered through pkg-config when available, otherwise fall
# back to its fltk-config helper.
fltk_dep = dependency('fltk', required: false)
if not fltk_dep.found()
  fltk_config = find_program('fltk-config', required: true)
  fltk_cflags = run_command(fltk_config, '--cflags', check: true).stdout().strip().split()
  fltk_ldflags = run_command(fltk_config, '--use-images', '--ldflags', check: true).stdout().strip().split()
  fltk_dep = declare_dependency(
    compile_args: fltk_cflags,
    link_args: fltk_ldflags,
  )
endif
threads_dep = dependency('threads', required: true)

# Install location of the gettext message catalogs (e.g. /usr/share/locale).
localedir = join_paths(get_option('prefix'), get_option('localedir'))
add_project_arguments('-DLOCALEDIR="' + localedir + '"', language: 'cpp')

core_inc = include_directories('src/core')
gui_inc = include_directories('src/gui')

sources = files(
  'src/main.cpp',
  'src/gui/i18n_strings.cpp',
  'src/gui/logo.cpp',
  'src/core/package.cpp',
  'src/core/xbps_command.cpp',
  'src/core/package_repository.cpp',
  'src/core/package_model.cpp',
  'src/core/category.cpp',
  'src/core/settings.cpp',
  'src/core/utils.cpp',
  'src/core/privileged_daemon.cpp',
  'src/gui/theme.cpp',
  'src/gui/main_window.cpp',
  'src/gui/main_window_callbacks.cpp',
  'src/gui/main_window_ui.cpp',
  'src/gui/main_window_packages.cpp',
  'src/gui/main_window_transaction.cpp',
  'src/gui/main_window_exec.cpp',
  'src/gui/main_window_actions.cpp',
  'src/gui/repository_editor.cpp',
)

executable('iruka-xbps',
  sources,
  include_directories: [core_inc, gui_inc],
  dependencies: [fltk_dep, threads_dep],
  install: true,
)

# Privileged helper daemon, launched once by the GUI through pkexec.
executable('iruka-helper',
  files('src/helper/main.cpp'),
  include_directories: [core_inc],
  install: true,
)

# Application icon: SVG (scalable, crisp) + PNG fallback for the hicolor theme.
# Installed as 'iruka' so the About dialog's set_logo_icon_name() and the
# Icon= field in iruka-xbps.desktop both resolve.
icon_resdir = join_paths(get_option('datadir'), 'icons', 'hicolor')
install_data('resources/icons/iruka.svg',
  install_dir: join_paths(icon_resdir, 'scalable', 'apps'),
  rename: 'iruka.svg')
install_data('resources/icons/iruka.png',
  install_dir: join_paths(icon_resdir, '96x96', 'apps'),
  rename: 'iruka.png')

# .desktop launcher.
install_data('iruka-xbps.desktop',
  install_dir: join_paths(get_option('datadir'), 'applications'))

# Translations (po/LINGUAS, po/POTFILES.in and po/<lang>.po).
subdir('po')
