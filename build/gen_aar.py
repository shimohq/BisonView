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
from functools import partial
import jinja2
from datetime import datetime

from bison_build_util import AddResources, MergeRTxt , MergeProguardConfigs ,AddAssets

SCRIPT_DIR = os.path.dirname(os.path.realpath(sys.argv[0]))
SRC_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, os.pardir, os.pardir))





DEFAULT_ARCHS = ['armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64']
TARGET = 'bison:bison_view_aar'
MANIFEST_FILE = 'gen/bison/bison_aar_manifest/AndroidManifest.xml'
AAR_CONFIG_FILE = os.path.join('gen',TARGET.replace(':','/'))+ ".build_config"

jar_excluded_patterns = [
  "im/shimo/bison/R\$*.class",
  "im/shimo/bison/R.class",

  "android/support/*",
  "*/third_party/android_deps/*",
  "androidx/*",
  "android/support/*",
  "gen/_third_party/*",
  "com/google/*",
  "javax/*",
  "META-INF/*",
  "*.txt",
  "*.properties"
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

GROUP_ID = 'im.shimo.bison'
ARTIFACT_ID = 'bisonview'



def _ParseArgs():
  parser = argparse.ArgumentParser(description='bison_view.aar generator.')

  parser.add_argument('--version-name', help='aar version' ,default='1.0.0')
  parser.add_argument('--build-type', default='debug',
      help='Build type. default debug')

  parser.add_argument('--build-dir', default='out',
      help='Build dir. default  out')
  # parser.add_argument('--output', default='bison_view.aar',
  #     help='Output file of the script.')
  parser.add_argument('--arch', default=DEFAULT_ARCHS, nargs='*',
      help='Architectures to build. Defaults to %(default)s.')
  parser.add_argument('-p','--publish' ,action='store_true',default=False ,
      help ='publish aar file to maven')
  parser.add_argument('-s','--snapshot', action='store_true',default=False ,
      help ='publish snapshot to maven')
  parser.add_argument('-v','--verbose', action='store_true', default=False,
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
  cmd = [os.path.join(find_depot_tools.DEPOT_TOOLS_PATH, 'ninja'),
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

def _RebasePath(path, build_dir, arch):
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
      # print ('input',in_file)
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
            # if not already_added:
            #   print("already_added:",already_added,dst_name,in_file)

          if "J/N.class" in dst_name and not already_added:
            already_added = not "bison" in in_file
            # if not already_added:
            #   print("already_added:",already_added,dst_name,in_file)

          # if "properties" in dst_name :
          #   print("merge :" +dst_name)
          if not already_added:
            # print("merge :" +dst_name)
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


def _ReadConfig(build_dir , arch, config, *args):
  values = build_utils.ParseGnList(reduce(lambda x, y : x.get(y),args, config))
  return [_RebasePath(  v,build_dir , arch ) for v in values]

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

def Build(build_dir, build_type, arch, common_gn_args, extra_gn_switches,
          extra_ninja_switches):
  """Generates target architecture using GN and builds it using ninja."""
  logging.info('Building: %s', arch)
  output_directory = _GetOutputDirectory(build_dir, arch)
  gn_args = {
    'target_cpu': _GetTargetCpu(arch),
  }
  gn_args_str = '--args=' + ' '.join([
      k + '=' + _EncodeForGN(v) for k, v in gn_args.items()]) + ' ' +common_gn_args
  gn_args_list = ['gen', output_directory, gn_args_str]
  gn_args_list.extend(extra_gn_switches)

  _RunGN(gn_args_list)

  ninja_args = [TARGET]
  ninja_args.extend(extra_ninja_switches)

  _RunNinja(output_directory, ninja_args)

def BuildAar(archs, output_file, common_gn_args,
             ext_build_dir=None, build_type = 'debug',
             extra_gn_switches=None,
             extra_ninja_switches=None):
  extra_gn_switches = extra_gn_switches or []
  extra_ninja_switches = extra_ninja_switches or []
  build_dir = ext_build_dir + "/" + build_type if ext_build_dir else tempfile.mkdtemp()

  isCommonArgsGetted = False

  for arch in archs:
    Build(build_dir, build_type,arch, common_gn_args, extra_gn_switches,
          extra_ninja_switches)

    if not isCommonArgsGetted :
      output_directory = _GetOutputDirectory(build_dir, arch)
      with open(os.path.join(output_directory , AAR_CONFIG_FILE),'r') as f :
        build_config = json.loads(f.read())
        jars = _ReadConfig(build_dir, arch , build_config,'deps_info', 'javac_full_classpath')
        dependencies_res_zips =_ReadConfig(build_dir, arch, build_config ,'deps_info', 'dependency_zips')
        r_text_files = _ReadConfig(build_dir, arch, build_config ,'deps_info', 'extra_r_text_files')
        proguard_configs = _ReadConfig(build_dir, arch, build_config ,'deps_info', 'proguard_all_configs')

      print("=== R ===")
      for rf in r_text_files:
        if "android_deps" in rf:
          print(rf)

      print("=== Res ===")
      for rf in dependencies_res_zips:
        if "android_deps" in rf:
          print(rf)

      print("=== jar ===")
      for rf in jars:
        if "android_deps" in rf:
          print(rf)

      isCommonArgsGetted = True


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
            data=MergeRTxt(r_text_files, resource_included_patterns))
      build_utils.AddToZipHermetic(aar_file, 'public.txt', data='')

      build_utils.AddToZipHermetic(
              aar_file, 'proguard.txt',
              data=MergeProguardConfigs(proguard_configs))

      AddResources(aar_file, dependencies_res_zips,
                      resource_included_patterns)

    for arch in archs:
      Collect(aar_file, build_dir, arch)
      deps_configs = _ReadConfig(build_dir, arch, build_config ,'deps_info', 'deps_configs')
      partial_rebase_path = partial(_RebasePath , build_dir = build_dir , arch = arch)
      AddAssets(aar_file, deps_configs, partial_rebase_path)

  if not ext_build_dir:
    shutil.rmtree(build_dir, True)

  logging.info('Build success: %s', output_file)

def _GeneratePom(target_file, version,args):
  env = jinja2.Environment(loader=jinja2.PackageLoader('gen_aar'))
  template = env.get_template('pom.jinja')
  deps = [
    {
      "groupId": "androidx.appcompat",
      "artifactId": "appcompat",
      "version" : "1.2.0",
      "type" : "aar"
    }
  ]

  pom = template.render(version=version,args=args ,deps = deps)
  print (pom)
  with open(target_file, 'w') as fh:
    fh.write(pom)


def publish(filename , verison , is_snapshot,common_gn_args):
  url = os.environ.get('SNAPSHOT_REPOSITORY_URL', None) if is_snapshot else os.environ.get('RELEASE_REPOSITORY_URL', None)
  pom_path = os.path.join(os.path.dirname(filename),os.path.splitext(os.path.basename(filename))[0]+'.pom')
  _GeneratePom(pom_path, verison, common_gn_args)
  cmd = ['mvn']
  args = {
    "-DgroupId":GROUP_ID,
    "-DartifactId":ARTIFACT_ID,
    "-Dversion": verison,
    "-DrepositoryId":"nexus" ,
    "-Durl": url ,
    "-Dfile":filename,
    "-DpomFile":pom_path,
    "-DgeneratePom":"false",
  }
  cmd.extend(['deploy:deploy-file'])
  cmd.extend(['{}={}'.format(*arg) for arg in args.items()])

  logging.info('Uploading: %s', filename)
  logging.info('maven cmd is: %s', " ".join(cmd))

  subprocess.check_call(cmd)


def createGnArgs(extra_gn_args,build_type):
  gn_args = {
    'target_os': 'android',
    'is_debug': 'debug'== build_type,
    'is_component_build': False,
    'rtc_include_tests': False,
    'v8_android_log_stdout' : 'debug'== build_type,
    # 'use_v8_context_snapshot' : True,
    'ffmpeg_branding' : 'Chrome',
    'proprietary_codecs' : True, # <audio/>
    # 'v8_embed_script' : '//bison/docHistory.bundle.js',
  }
  return ' '.join([
      k + '=' + _EncodeForGN(v) for k, v in gn_args.items()] + extra_gn_args)



def main():
  args = _ParseArgs()
  logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

  common_gn_args = createGnArgs(args.extra_gn_args,args.build_type)

  verison = args.version_name
  if args.publish and args.snapshot:
    verison = verison + "-SNAPSHOT"

  base_name = ARTIFACT_ID + '-' + verison

  output = os.path.join(args.build_dir, args.build_type, base_name+".aar")
  BuildAar(args.arch, output, common_gn_args,
           args.build_dir,args.build_type, args.extra_gn_switches, args.extra_ninja_switches)
  if args.publish:
    gn_args = common_gn_args if args.snapshot else None;
    publish(output, verison, args.snapshot, common_gn_args)





if __name__ == '__main__':
  sys.exit(main())
