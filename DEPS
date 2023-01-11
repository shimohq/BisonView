
gclient_gn_args_file = 'src/build/config/gclient_args.gni'
gclient_gn_args = [
  'build_with_chromium',
  'checkout_android',
  'checkout_android_prebuilts_build_tools',
  'checkout_android_native_support',
  'checkout_google_benchmark',
  'checkout_ios_webkit',
  'checkout_nacl',
  'checkout_oculus_sdk',
  'checkout_openxr',
  'mac_xcode_version',
]


vars = {
  "buildspec_platforms": "android",
  'build_with_chromium': True,
  'checkout_android': True,
  'checkout_android_prebuilts_build_tools': True,
  'checkout_android_native_support': True,
  'checkout_google_benchmark': False,
  'checkout_ios_webkit': False,
  'checkout_nacl': True,
  'checkout_oculus_sdk' : False,
  'checkout_openxr' : False,
  'mac_xcode_version': 'default',

  'chromium_git': 'https://chromium.googlesource.com',
  'chromium_version' : '103.0.5060.129',
}

deps = {
  'src':
    Var('chromium_git') + '/chromium/src.git@' + Var('chromium_version'),
}

hooks = [
  {
    'name': 'patch_chromium',
    'pattern': 'src',
    'action': [
      'echo patch_chromium',
      '&&',
      'git',
      'apply',
      'bison/patchs/chromium.patch'
    ],
  },
]


recursedeps = [
  'src',
]
