i18n = import('i18n')

# Generates iruka-xbps.pot, per-language .mo catalogs and installs them under
# the configured localedir. POTFILES.in lists the sources to extract from and
# LINGUAS lists the available languages.
i18n.gettext(meson.project_name(),
  args: ['--from-code=UTF-8', '--keyword=N_:1', '--add-comments'],
  preset: 'glib',
)
