#!/usr/bin/env python
# coding=utf-8 
#
# Author jiangshiqu
#
# ref://build/android/gyp/dist_aar.py
# Currently supports:
#   * AndroidManifest.xml
#   * classes.jar
#   * jni/
#   * res/
#   * R.txt
#   * proguard.txt
#   * assets/  暂时根据bison写的固定值
# Does not yet support:
#   * public.txt
#   * annotations.zip

"""Creates an Android .aar file."""

import argparse
import os
import posixpath
import shutil
import sys
import tempfile
import zipfile
import json


GYP_ANDROID_DIR = os.path.join(os.path.dirname(__file__),
                               os.pardir, os.pardir,
                               'build',
                               'android',
                               'gyp')
sys.path.append(GYP_ANDROID_DIR)

from filter_zip import CreatePathTransform
from util import build_utils
from bison_build_util import AddResources, MergeRTxt , MergeProguardConfigs ,AddAssets


_ANDROID_BUILD_DIR = os.path.dirname(os.path.dirname(__file__))


def main(args):
  args = build_utils.ExpandFileArgs(args)
  parser = argparse.ArgumentParser()

  build_utils.AddDepfileOption(parser)
  parser.add_argument('--output', required=True, help='Path to output aar.')
  # parser.add_argument('--native-jars', required=True, help='GN list of jar inputs.')
  parser.add_argument('--jars', required=True, help='GN list of jar inputs.')
  parser.add_argument('--dependencies-res-zips', required=True,
                      help='GN list of resource zips')
  parser.add_argument('--r-text-files', required=True,
                      help='GN list of R.txt files to merge')
  parser.add_argument('--proguard-configs', required=True,
                      help='GN list of ProGuard flag files to merge.')
  parser.add_argument('--deps-configs', required=False,
                      help='GN list of ProGuard flag files to merge.')

  parser.add_argument(
      '--android-manifest',
      help='Path to AndroidManifest.xml to include.',
      default=os.path.join(_ANDROID_BUILD_DIR, 'AndroidManifest.xml'))
  parser.add_argument('--native-libraries', default='',
                      help='GN list of native libraries. If non-empty then '
                      'ABI must be specified.')
  parser.add_argument('--abi',
                      help='ABI (e.g. armeabi-v7a) for native libraries.')
  parser.add_argument(
      '--jar-excluded-globs',
      help='GN-list of globs for paths to exclude in jar.')
  parser.add_argument(
      '--jar-included-globs',
      help='GN-list of globs for paths to include in jar.')
  parser.add_argument(
      '--resource-included-globs',
      help='GN-list of globs for paths to include in R.txt and resources zips.')
  
  options = parser.parse_args(args)

  

  if options.native_libraries and not options.abi:
    parser.error('You must provide --abi if you have native libs')

  options.jars = build_utils.ParseGnList(options.jars)
  options.dependencies_res_zips = build_utils.ParseGnList(
      options.dependencies_res_zips)
  options.r_text_files = build_utils.ParseGnList(options.r_text_files)
  options.proguard_configs = build_utils.ParseGnList(options.proguard_configs)
  options.native_libraries = build_utils.ParseGnList(options.native_libraries)
  options.jar_excluded_globs = build_utils.ParseGnList(
      options.jar_excluded_globs)
  options.jar_included_globs = build_utils.ParseGnList(
      options.jar_included_globs)
  options.resource_included_globs = build_utils.ParseGnList(
      options.resource_included_globs)

  options.deps_configs= build_utils.ParseGnList(options.deps_configs)

  with tempfile.NamedTemporaryFile(delete=False) as staging_file:
    try:
      with zipfile.ZipFile(staging_file.name, 'w') as z:
        build_utils.AddToZipHermetic(
            z, 'AndroidManifest.xml', src_path=options.android_manifest)

        path_transform = CreatePathTransform(options.jar_excluded_globs,
                                             options.jar_included_globs, [])
        with tempfile.NamedTemporaryFile() as jar_file:
          build_utils.MergeZips(
              jar_file.name, options.jars, path_transform=path_transform)
          build_utils.AddToZipHermetic(z, 'classes.jar', src_path=jar_file.name)

        build_utils.AddToZipHermetic(
            z,
            'R.txt',
            data=MergeRTxt(options.r_text_files,
                            options.resource_included_globs))           
        build_utils.AddToZipHermetic(z, 'public.txt', data='')

        if options.proguard_configs:
          build_utils.AddToZipHermetic(
              z, 'proguard.txt',
              data=MergeProguardConfigs(options.proguard_configs))

        AddResources(z, options.dependencies_res_zips,
                      options.resource_included_globs)

        for native_library in options.native_libraries:
          libname = os.path.basename(native_library)
          build_utils.AddToZipHermetic(
              z, os.path.join('jni', options.abi, libname),
              src_path=native_library)
          AddAssets(z, options.deps_configs)
          
    except:
      os.unlink(staging_file.name)
      raise
    shutil.move(staging_file.name, options.output)

  if options.depfile:
    all_inputs = (options.jars + options.dependencies_res_zips +
                  options.r_text_files + options.proguard_configs)
    build_utils.WriteDepfile(options.depfile, options.output, all_inputs)


if __name__ == '__main__':
  main(sys.argv[1:])
