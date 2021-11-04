
edc_template_files = [
  'Basic.edc',
  'BgAnimation.edc',
  'Clip.edc',
  'CustomButton.edc',
  'Empty.edc',
  'GameStage.edc',
  'Group.edc',
  'Image.edc',
  'Interpolation.edc',
  'Listitem.edc',
  'Map.edc',
  'Mask.edc',
  'MobileLayout.edc',
  'Morphing.edc',
  'MouseEvent.edc',
  'Proxy.edc',
  'Rect.edc',
  'Rotation+Zoom.edc',
  'Sound.edc',
  'Spacer.edc',
  'Text.edc',
  'Textblock.edc',
  'Transition.edc',
  'Tween.edc',
  'Vector.edc',
  'Watchface.edc',
  'Watchface2.edc'
]

enventor_templates = []

foreach edc_file : edc_template_files
  enventor_templates += custom_target('edje_cc_' + edc_file,
    input       : edc_file,
    depfile     : '@BASENAME@.edj.d',
    output      : '@BASENAME@.edj',
    command     : edje_cc_exe +
                  [
                   '-beta',
                   '-sd', join_paths(meson.source_root(), 'data/sounds'),
                   '-id', join_paths(meson.source_root(), 'data/images'),
                   '@INPUT@', '@OUTPUT@'
                  ],
    install     : true,
    install_dir : join_paths(dir_pkgdata, 'templates'),
  )
endforeach

install_data(
  sources     : edc_template_files,
  install_dir : join_paths(dir_pkgdata, 'templates')
)
