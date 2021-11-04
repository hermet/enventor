
custom_target('autocomp.eet',
  input       : 'autocomp.src',
  output      : 'autocomp.eet',
  command     : eet_exe + [
                            '-e',
                            join_paths(meson.current_build_dir(), 'autocomp.eet'),
                            'node',
                            join_paths(meson.current_source_dir(), 'autocomp.src'),
                            '1'
                          ],
  install     : true,
  install_dir : join_paths(dir_pkgdata, 'autocomp')
)
