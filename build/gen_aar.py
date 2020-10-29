#!/usr/bin/env python

"""Script to generate libbison_view.aar for distribution.
The script has to be run from the root src folder.
./bison/build/build_aar.py
.aar-file is just a zip-archive containing the files of the library. The file

structure generated by this script looks like this:
 - assets /
 
 - jni/
   - armeabi-v7a/
     - libbison_view.so
   - x86/
     - libbison_view.so
 - classes.jar
 - AndroidManifest.xml


"""

import argparse
import logging
import os
import posixpath
import shutil
import subprocess
import sys
import tempfile
import zipfile
import json

SCRIPT_DIR = os.path.dirname(os.path.realpath(sys.argv[0]))
SRC_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, os.pardir, os.pardir))
DEFAULT_ARCHS = ['armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64']

MANIFEST_FILE = 'gen/bison/aar/bison_aar_manifest/AndroidManifest.xml'
AAR_CONFIG_FILE = 'gen/bison/aar/bison_view_aar.build_config'

TARGETS = [
  'bison/aar:bison_view_aar'
]

jar_excluded_patterns = [
  "im/shimo/bison/R\$*.class",
  "im/shimo/bison/R.class",

  "javax/*",
  "android/*",
  "androidx/*",
  "com/google/*",
  "META-INF/*",
  "*.stamp",
  "*.readme",
  "*.properties",
]

resource_included_patterns = [
  "*/bison/*",
  "*/ui/android/*",
  "*/content/*",
]

NEEDED_SO_FILES = [
  "libbison_view.so"
]

sys.path.append(os.path.join(SRC_DIR, 'build'))
import find_depot_tools

GYP_ANDROID_DIR = os.path.join(os.path.dirname(__file__),
                               os.pardir, os.pardir,
                               'build',
                               'android',
                               'gyp')
sys.path.append(GYP_ANDROID_DIR)

from filter_zip import CreatePathTransform
from util import build_utils



def _ParseArgs():
  parser = argparse.ArgumentParser(description='bison_view.aar generator.')

  parser.add_argument('--verison-name', help='aar verison')
  parser.add_argument('--build-type', default='debug',
      help='Build type. default debug')

  parser.add_argument('--build-dir', default='out',
      help='Build dir. default  out')
  parser.add_argument('--output', default='bison_view.aar',
      help='Output file of the script.')
  parser.add_argument('--arch', default=DEFAULT_ARCHS, nargs='*',
      help='Architectures to build. Defaults to %(default)s.')


  

  parser.add_argument('--verbose', action='store_true', default=False,
      help='Debug logging.')

  parser.add_argument('--extra-gn-args', default=[], nargs='*',
      help="""Additional GN arguments to be used during Ninja generation.
              These are passed to gn inside `--args` switch and
              applied after any other arguments and will
              override any values defined by the script.
              Example of building debug aar file:
              build_aar.py --extra-gn-args='is_debug=true'""")
  parser.add_argument('--extra-ninja-switches', default=[], nargs='*',
      help="""Additional Ninja switches to be used during compilation.
              These are applied after any other Ninja switches.
              Example of enabling verbose Ninja output:
              build_aar.py --extra-ninja-switches='-v'""")
  parser.add_argument('--extra-gn-switches', default=[], nargs='*',
      help="""Additional GN switches to be used during compilation.
              These are applied after any other GN switches.
              Example of enabling verbose GN output:
              build_aar.py --extra-gn-switches='-v'""")
  return parser.parse_args()


def _RunGN(args):
  cmd = [sys.executable,
         os.path.join(find_depot_tools.DEPOT_TOOLS_PATH, 'gn.py')]
  cmd.extend(args)
  logging.debug('Running: %r', cmd)
  subprocess.check_call(cmd)


def _RunNinja(output_directory, args):
  cmd = [os.path.join(find_depot_tools.DEPOT_TOOLS_PATH, 'autoninja'),
         '-C', output_directory]
  cmd.extend(args)
  logging.debug('Running: %r', cmd)
  subprocess.check_call(cmd)


def _EncodeForGN(value):
  """Encodes value as a GN literal."""
  if isinstance(value, str):
    return '"' + value + '"'
  elif isinstance(value, bool):
    return repr(value).lower()
  else:
    return repr(value)

def _GetOutputDirectory(build_dir, arch):
  """Returns the GN output directory for the target architecture."""
  return os.path.join(build_dir, arch)

def _RebasePath(build_dir, arch, path):
  return os.path.join(_GetOutputDirectory(build_dir, arch),path)

def _GetTargetCpu(arch):
  """Returns target_cpu for the GN build with the given architecture."""
  if arch in ['arm','armeabi', 'armeabi-v7a']:
    return 'arm'
  elif arch == 'arm64-v8a':
    return 'arm64'
  elif arch == 'x86':
    return 'x86'
  elif arch == 'x86_64':
    return 'x64'
  else:
    raise Exception('Unknown arch: ' + arch)


def MergeZips(output, input_zips, path_transform=None, compress=None):
  """Combines all files from |input_zips| into |output|.

  Args:
    output: Path, fileobj, or ZipFile instance to add files to.
    input_zips: Iterable of paths to zip files to merge.
    path_transform: Called for each entry path. Returns a new path, or None to
        skip the file.
    compress: Overrides compression setting from origin zip entries.
  """

  
  path_transform = path_transform or (lambda p: p)
  added_names = set()

  out_zip = output
  if not isinstance(output, zipfile.ZipFile):
    out_zip = zipfile.ZipFile(output, 'w')

  try:
    for in_file in input_zips:
      print ('input',in_file)
      with zipfile.ZipFile(in_file, 'r') as in_zip:
        # ijar creates zips with null CRCs.
        in_zip._expected_crc = None
        for info in in_zip.infolist():
          # Ignore directories.
          if info.filename[-1] == '/':
            continue
          dst_name = path_transform(info.filename)
          if not dst_name:
            continue
          already_added = dst_name in added_names
          # TODO  jiang gen_jni filter for bison
          if "GEN_JNI" in dst_name and not already_added:
            already_added = not "bison" in in_file
            if not already_added:
              print("already_added:",already_added,dst_name,in_file)

          if "J/N.class" in dst_name and not already_added:
            already_added = not "bison" in in_file
            if not already_added:
              print("already_added:",already_added,dst_name,in_file)

          if "properties" in dst_name :
            print("merge :" +dst_name)
          if not already_added:
            if compress is not None:
              compress_entry = compress
            else:
              compress_entry = info.compress_type != zipfile.ZIP_STORED
            build_utils.AddToZipHermetic(
                out_zip,
                dst_name,
                data=in_zip.read(info),
                compress=compress_entry)
            added_names.add(dst_name)
  finally:
    if output is not out_zip:
      out_zip.close()

def _MergeRTxt(r_paths, include_globs):
  """Merging the given R.txt files and returns them as a string."""
  all_lines = set()
  keys = []
  for r_path in r_paths:
    if include_globs and not build_utils.MatchesGlob(r_path, include_globs):
      continue
    lines = []
    with open(r_path) as f:
      for line in f.readlines():
         key = " ".join(line.split(' ')[:3])
         if key not in keys :
           keys.append(key)
           lines.append(line)
      
      all_lines.update(lines)
  return ''.join(sorted(all_lines))

def _MergeProguardConfigs(proguard_configs):
  """Merging the given proguard config files and returns them as a string."""
  ret = []
  for config in proguard_configs:
    ret.append('# FROM: {}'.format(config))
    with open(config) as f:
      ret.append(f.read())
  return '\n'.join(ret)


def _AddResources(aar_zip, resource_zips, include_globs):
  """Adds all resource zips to the given aar_zip.

  Ensures all res/values/* files have unique names by prefixing them.
  """
  for i, path in enumerate(resource_zips):
    print("res path" , path)
    if include_globs and not build_utils.MatchesGlob(path, include_globs):
      continue
    with zipfile.ZipFile(path) as res_zip:
      for info in res_zip.infolist():
        data = res_zip.read(info)
        dirname, basename = posixpath.split(info.filename)
        if 'values' in dirname:
          root, ext = os.path.splitext(basename)
          basename = '{}_{}{}'.format(root, i, ext)
          info.filename = posixpath.join(dirname, basename)
        info.filename = posixpath.join('res', info.filename)
        aar_zip.writestr(info, data)

def _AddAssets(aar_zip, deps_configs ,build_dir, arch):
  for config_path in deps_configs:
    if not "assets" in config_path :
      continue
    with open(config_path,'r') as f:
      config = json.loads(f.read())
      sources = reduce(lambda x, y : x.get(y),["deps_info","assets","sources"], config)
      sources_rebase =  [_RebasePath( build_dir , arch , v) for v in sources]
      outputs = config.get("deps_info").get("assets").get("outputs") or sources
 
      for source , output in zip(sources_rebase,outputs) :
        zip_path = os.path.join("assets",output)
        if zip_path in aar_zip.namelist():
          print (zip_path +" in namelist")
          continue
        build_utils.AddToZipHermetic(
          aar_zip,os.path.join("assets",output),src_path=source)
      

def _ReadConfig(build_dir , arch, config, *args):
  values = build_utils.ParseGnList(reduce(lambda x, y : x.get(y),args, config))
  return [_RebasePath( build_dir , arch , v) for v in values]

def AddAndroidManifest(aar_file, build_dir, arch):
  """Collects architecture independent files into the .aar-archive."""
  logging.info('Collecting common files.')
  output_directory = _GetOutputDirectory(build_dir, arch)
  aar_file.write(os.path.join(output_directory, MANIFEST_FILE), 'AndroidManifest.xml')

def Collect(aar_file, build_dir, arch):
  """Collects architecture specific files into the .aar-archive."""
  logging.info('Collecting: %s', arch)
  output_directory = _GetOutputDirectory(build_dir, arch)
  abi_dir = os.path.join('jni', 'armeabi-v7a' if arch in ['arm','armeabi', 'armeabi-v7a'] else arch)
  for so_file in NEEDED_SO_FILES:
    aar_file.write(os.path.join(output_directory, so_file),
                   os.path.join(abi_dir, so_file))

def Build(build_dir, build_type, arch, extra_gn_args, extra_gn_switches,
          extra_ninja_switches,verison_name = "1.0.0"):
  """Generates target architecture using GN and builds it using ninja."""
  logging.info('Building: %s', arch)
  output_directory = _GetOutputDirectory(build_dir, arch)
  gn_args = {
    'target_os': 'android',
    'is_debug': 'debug'== build_type,
    'is_component_build': False,
    'rtc_include_tests': False,
    'target_cpu': _GetTargetCpu(arch),
    'android_override_version_name' : verison_name,
  }
  
  gn_args_str = '--args=' + ' '.join([
      k + '=' + _EncodeForGN(v) for k, v in gn_args.items()] + extra_gn_args)

  gn_args_list = ['gen', output_directory, gn_args_str]
  gn_args_list.extend(extra_gn_switches)
  _RunGN(gn_args_list)

  ninja_args = TARGETS[:]
  ninja_args.extend(extra_ninja_switches)
  _RunNinja(output_directory, ninja_args)

def BuildAar(archs, output_file, extra_gn_args=None,
             ext_build_dir=None, build_type = 'debug',
             extra_gn_switches=None,
             extra_ninja_switches=None,
             verison_name = "1.0.0"):
  extra_gn_args = extra_gn_args or []
  extra_gn_switches = extra_gn_switches or []
  extra_ninja_switches = extra_ninja_switches or []
  build_dir = ext_build_dir + "/" + build_type if ext_build_dir else tempfile.mkdtemp()
  


  isCommonArgsGetted = False

  for arch in archs:
    Build(build_dir, build_type,arch, extra_gn_args, extra_gn_switches,
          extra_ninja_switches,verison_name)

    if not isCommonArgsGetted :
      output_directory = _GetOutputDirectory(build_dir, arch)

      with open(os.path.join(output_directory,AAR_CONFIG_FILE),'r') as f :
        build_config = json.loads(f.read())
        # classpath = build_config.get('javac').get('classpath')
        jars = _ReadConfig(build_dir, arch , build_config,'deps_info', 'javac_full_classpath') 
        dependencies_res_zips =_ReadConfig(build_dir, arch, build_config ,'deps_info', 'dependency_zips') 
        r_text_files = _ReadConfig(build_dir, arch, build_config ,'deps_info', 'extra_r_text_files') 
        proguard_configs = _ReadConfig(build_dir, arch, build_config ,'deps_info', 'proguard_all_configs') 
        


  with zipfile.ZipFile(output_file, 'w') as aar_file:
    path_transform = CreatePathTransform(jar_excluded_patterns,
                                             [], [])

    AddAndroidManifest(aar_file, build_dir, archs[0])

    with tempfile.NamedTemporaryFile() as jar_file:
      MergeZips(jar_file.name, jars, path_transform=path_transform)
      build_utils.AddToZipHermetic(aar_file, 'classes.jar', src_path=jar_file.name)

      build_utils.AddToZipHermetic(
            aar_file,
            'R.txt',
            data=_MergeRTxt(r_text_files, resource_included_patterns))           
      build_utils.AddToZipHermetic(aar_file, 'public.txt', data='')

      build_utils.AddToZipHermetic(
              aar_file, 'proguard.txt',
              data=_MergeProguardConfigs(proguard_configs))

      _AddResources(aar_file, dependencies_res_zips,
                      resource_included_patterns)

      

    for arch in archs:
      Collect(aar_file, build_dir, arch)
      deps_configs = _ReadConfig(build_dir, arch, build_config ,'deps_info', 'deps_configs') 
      _AddAssets(aar_file, deps_configs, build_dir, arch)

  if not ext_build_dir:
    shutil.rmtree(build_dir, True)

  logging.info('Build success: %s', output_file)

def main():
  args = _ParseArgs()
  logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
  output = os.path.join(args.build_dir,args.build_type,args.output)
  BuildAar(args.arch, output, args.extra_gn_args,
           args.build_dir,args.build_type, args.extra_gn_switches, args.extra_ninja_switches,
           args.verison_name)


if __name__ == '__main__':
  sys.exit(main())
  

  

    


      
