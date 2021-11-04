
custom_target('color.eet',
  input       : 'edc.src',
  output      : 'edc.eet',
  command     : eet_exe + [
                            '-e',
                            join_paths(meson.current_build_dir(), 'edc.eet'),
                            'color',
                            join_paths(meson.current_source_dir(), 'edc.src'),
                            '1'
                          ],
  install     : true,
  install_dir : join_paths(dir_pkgdata, 'color')
)

install_data(
  sources     : 'syntax_template.dat',
  install_dir : join_paths(dir_pkgdata, 'color')
)
