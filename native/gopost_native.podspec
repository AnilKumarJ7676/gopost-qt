Pod::Spec.new do |s|
  s.name             = 'gopost_native'
  s.version          = '0.1.0'
  s.summary          = 'GoPost native image/video engine'
  s.homepage         = 'https://gopost.app'
  s.license          = { :type => 'Proprietary' }
  s.author           = 'GoPost'
  s.source           = { :path => '.' }

  s.ios.deployment_target = '13.0'
  s.osx.deployment_target = '10.15'

  s.source_files = [
    'gopost_core/include/**/*.h',
    'gopost_image_engine/include/**/*.h',
    'gopost_video_engine/include/**/*.h',
    'gopost_core/src/**/*.{cpp,c,h}',
    'gopost_image_engine/src/**/*.{cpp,h}',
    'gopost_image_engine/src/gpu/metal/metal_renderer.cpp',
    'gopost_video_engine/src/**/*.{cpp,mm,hpp}',
  ]

  s.public_header_files = [
    'gopost_core/include/**/*.h',
    'gopost_image_engine/include/**/*.h',
    'gopost_video_engine/include/**/*.h',
  ]

  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => [
      '"$(PODS_TARGET_SRCROOT)/gopost_core/include"',
      '"$(PODS_TARGET_SRCROOT)/gopost_core/src"',
      '"$(PODS_TARGET_SRCROOT)/gopost_image_engine/include"',
      '"$(PODS_TARGET_SRCROOT)/gopost_image_engine/src"',
      '"$(PODS_TARGET_SRCROOT)/gopost_video_engine/include"',
      '"$(PODS_TARGET_SRCROOT)/gopost_video_engine/src"',
      '"$(PODS_TARGET_SRCROOT)/gopost_video_engine/src/video"',
    ].join(' '),
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++20',
    'CLANG_CXX_LIBRARY'           => 'libc++',
    'GCC_C_LANGUAGE_STANDARD'     => 'c17',
    'GCC_OPTIMIZATION_LEVEL'      => '3',
    'LLVM_LTO'                    => 'YES',
    'OTHER_CFLAGS'                => '-DGOPOST_PLATFORM_IOS -DGOPOST_GPU_metal -DNDEBUG',
    'OTHER_CPLUSPLUSFLAGS'        => '$(inherited) -O3',
  }

  s.frameworks = 'Metal', 'CoreGraphics', 'CoreFoundation', 'ImageIO',
                 'Security', 'AVFoundation', 'CoreMedia', 'AudioToolbox'
  s.library = 'c++'
end
