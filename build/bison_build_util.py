
import os
import sys
import posixpath
import shutil
import zipfile
import json
from functools import reduce


GYP_ANDROID_DIR = os.path.join(os.path.dirname(__file__),
                               os.pardir, os.pardir,
                               'build',
                               'android',
                               'gyp')
sys.path.append(GYP_ANDROID_DIR)
from util import build_utils


def AddAssets(aar_zip, deps_configs, arch_transform = None):
  for config_path in deps_configs:
    if not "assets" in config_path :
      continue
    with open(config_path,'r') as f:
      config = json.loads(f.read())
      sources = reduce(lambda x, y : x.get(y),["deps_info","assets","sources"], config)
      outputs = config.get("deps_info").get("assets").get("outputs") or sources
      if arch_transform :
        sources = [arch_transform(v) for v in sources]

      for source , output in zip(sources , outputs) :
        zip_path = os.path.join( "assets" , output)
        if zip_path in aar_zip.namelist():
          # print (zip_path +" in namelist")
          continue
        build_utils.AddToZipHermetic(
          aar_zip,os.path.join("assets",output),src_path=source)



def AddResources(aar_zip, resource_zips, include_globs):
  """Adds all resource zips to the given aar_zip.

  Ensures all res/values/* files have unique names by prefixing them.
  """
  for i, path in enumerate(resource_zips):
    if include_globs and not build_utils.MatchesGlob(path, include_globs):
      continue
    print("res path" , path)
    with zipfile.ZipFile(path) as res_zip:
      for info in res_zip.infolist():
        data = res_zip.read(info)
        info.filename = info.filename.replace('0_res','res', 1 ) if info.filename.startswith('0_res') else info.filename
        dirname, basename = posixpath.split(info.filename)
        if 'values' in dirname:
          root, ext = os.path.splitext(basename)
          basename = '{}_{}{}'.format(root, i, ext)
          info.filename = posixpath.join(dirname, basename)

        if not info.filename.startswith('res'):
          info.filename = posixpath.join('res', info.filename)

        aar_zip.writestr(info, data)

def MergeProguardConfigs(proguard_configs):
  """Merging the given proguard config files and returns them as a string."""
  ret = []
  for config in proguard_configs:
    ret.append('# FROM: {}'.format(config))
    with open(config) as f:
      ret.append(f.read())
  return '\n'.join(ret)

def MergeRTxt(r_paths, include_globs):
  """Merging the given R.txt files and returns them as a string."""
  all_lines = set()
  keys = []
  for r_path in r_paths:
    if include_globs and not build_utils.MatchesGlob(r_path, include_globs):
      continue
    lines = []
    with open(r_path) as f:
      print("will write r_path:" + r_path)
      for line in f.readlines():
         key = " ".join(line.split(' ')[:3])
         if key not in keys :
           keys.append(key)
           lines.append(line)
      all_lines.update(lines)
  return ''.join(sorted(all_lines))
