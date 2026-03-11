project('wordpad', 'c',
  version: '1.0.0',
  default_options: ['c_std=c17', 'warning_level=2'],
)

gtk4 = dependency('gtk4', version: '>= 4.14')
libadwaita = dependency('libadwaita-1', version: '>= 1.5')

sources = files(
  'src/main.c',
  'src/mainwindow.c',
  'src/toolbar.c',
  'src/dialogs.c',
  'src/rtf.c',
)

executable('wordpad',
  sources,
  dependencies: [gtk4, libadwaita],
  install: true,
)
