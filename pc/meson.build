
pkgconf = configuration_data()

pkgconf.set('prefix', get_option('prefix'))
pkgconf.set('exec_prefix', '${prefix}')
pkgconf.set('libdir', '${prefix}/@0@'.format(get_option('libdir')))
pkgconf.set('includedir', '${prefix}/@0@'.format(get_option('includedir')))
pkgconf.set('pkgincludedir', '${prefix}/@0@'.format(get_option('includedir')) + '/enventor')
pkgconf.set('ENVENTOR_VERSION_MAJOR', v_maj)
pkgconf.set('VERSION', meson.project_version())
pkgconf.set('requirement_enventor', requirement_enventor_pc)

pkg_install_dir = '@0@/pkgconfig'.format(get_option('libdir'))

configure_file(
  input : join_paths(meson.source_root(), 'pc/enventor.pc.in'),
  output : 'enventor.pc',
  configuration : pkgconf,
  install_dir : pkg_install_dir
)
