#!/usr/bin/env python

import argparse
import json
import logging
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import requests
import jinja2

SCRIPT_DIR = os.path.dirname(os.path.realpath(sys.argv[0]))

BISON_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir))

sys.path.append(os.path.join(BISON_ROOT, 'build'))
from gen_aar import BuildAar

GROUP_ID = 'im.shimo.bison'
ARTIFACT_ID = 'bisonview'


def _GetCommitHash():
  commit_hash = subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                                        cwd=BISON_ROOT).strip()
  return commit_hash


def _GeneratePom(target_file, version):
  env = jinja2.Environment(loader=jinja2.PackageLoader('publish'))
  template = env.get_template('pom.jinja')
  pom = template.render(version=version)
  with open(target_file, 'w') as fh:
    fh.write(pom)

def _UploadFile(user, password, filename, version, target_file):
  # URL is of format:
  # <repository_api>/<version>/<group_id>/<artifact_id>/<version>/<target_file>

  target_dir = version + '/' + GROUP_ID + '/' + ARTIFACT_ID + '/' + version
  target_path = target_dir + '/' + target_file
  url = CONTENT_API + '/' + target_path

  logging.info('Uploading %s to %s', filename, url)
  with open(filename) as fh:
    file_data = fh.read()

  for attempt in xrange(UPLOAD_TRIES):
    try:
      response = requests.put(url, data=file_data, auth=(user, password),
                              timeout=API_TIMEOUT_SECONDS)
      break
    except requests.exceptions.Timeout as e:
      logging.warning('Timeout while uploading: %s', e)
      time.sleep(UPLOAD_RETRY_BASE_SLEEP_SECONDS ** attempt)
  else:
    raise Exception('Failed to upload %s' % filename)

  if not response.ok:
    raise Exception('Failed to upload %s. Response: %s' % (filename, response))
  logging.info('Uploaded %s: %s', filename, response)

def main():
  publishAar()


def publishAar():
  version = "1.0." + "0"
  env = jinja2.Environment(loader=jinja2.PackageLoader('publish'))
  template = env.get_template('pom.jinja')
  user = os.environ.get('NEXUS_USERNAME', None)
  passwd = os.environ.get('NEXUS_PASSWORD', None)
  repository_url = os.environ.get("SNAPSHOT_REPOSITORY_URL",None)
  if not user or not passwd:
    raise Exception('Environment variables NEXUS_USERNAME and NEXUS_PASSWORD '
                    'must be defined.')

  
  
  try:
    build_dir = 'out'
    build_type = 'release'
    archs = ['arm', 'x86']
    base_name = ARTIFACT_ID + '-' + version
    aar_file = os.path.join(build_dir, base_name + '.aar')
    pom_file = os.path.join(build_dir, base_name + '.pom')
    output = os.path.join(build_dir, build_type, aar_file)
    BuildAar(archs, output, ext_build_dir="out", build_type=build_type)
    _GeneratePom(pom_file, version)
    
  finally:
    pass


if __name__ == '__main__':
  sys.exit(main())
