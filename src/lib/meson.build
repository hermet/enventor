
enventor_header_src = [ 'Enventor.h' ]

install_headers(enventor_header_src,
  install_dir : dir_pkginclude + '-' + v_maj
)

enventor_src = [
  'auto_comp.c',
  'build.c',
  'ctxpopup.c',
  'dummy_obj.c',
  'edc_editor.c',
  'edc_parser.c',
  'edj_mgr.c',
  'edj_viewer.c',
  'enventor_main.c',
  'enventor_smart.c',
  'redoundo.c',
  'reference.c',
  'syntax_color.c',
  'syntax_helper.c',
  'syntax_indent.c',
  'template.c',
  'util.c',
  'wireframes_obj.c'
]

enventor_lib = library('enventor', enventor_src,
  c_args : [ enventor_cargs,
             '-DHAVE_CONFIG_H',
             '-DENVENTOR_WIN32_BUILD_SUPPORT',
             '-I' + join_paths(meson.source_root(), 'src/include')
           ],
  dependencies : enventor_deps,
  include_directories : config_dir,
  install : true,
  version : meson.project_version()
)

enventor = declare_dependency(
  include_directories : [ include_directories('.')],
  link_with : enventor_lib,
  dependencies : enventor_deps
)
