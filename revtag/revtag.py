#!/usr/bin/env python
# WinKexec: kexec for Windows
# Copyright (C) 2008-2009 John Stumpo
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys
import os

# List of macros to define in the generated header files
macro_list = []

# Generate a header file and return the contents.
def make_header(prefix):
  innards = ''
  for key, value in macro_list:
    innards += '%sdefine %s %s\n' % (prefix, key, value)
  return '''%sifndef REVTAG_REVISION_HEADER
%sdefine REVTAG_REVISION_HEADER

%s
%sendif
''' % (prefix, prefix, innards, prefix)

def usage():
  sys.stderr.write('''usage: %s [operation] [path1] [path2]

operation: --make-headers or --tag-file
  --make-headers: write C and NSIS header files containing revision number
                  into folder named by path2
  --tag-file:     substitute revision number into file named by path2
                  and write results to stdout

path1: path to get the revision number from
path2: path to actually operate on
''' % sys.argv[0])
  sys.exit(1)

def notify(msg):
  sys.stderr.write('%s: %s\n' % (sys.argv[0], msg))

if len(sys.argv) != 4:
  usage()

def get_revnum():
  entries = open(os.path.join(sys.argv[2], '.svn', 'entries'), 'r')
  entries.readline()
  entries.readline()
  entries.readline()
  revnum = int(entries.readline().strip())
  macro_list.append(('SVN_REVISION', str(revnum)))
  macro_list.append(('SVN_REVISION_STR', '"%s"' % str(revnum)))
  entries.close()
  return revnum

if sys.argv[1] == '--make-headers':
  get_revnum()
  os.chdir(sys.argv[3])
  revtag_h = make_header('#')
  revtag_nsh = make_header('!')
  try:
    old_revtag_h = open('revtag.h', 'r').read()
  except:
    old_revtag_h = None
  try:
    old_revtag_nsh = open('revtag.nsh', 'r').read()
  except:
    old_revtag_nsh = None
  if revtag_h == old_revtag_h:
    notify('revtag.h is up to date')
  else:
    notify('updating revtag.h')
    open('revtag.h', 'w').write(revtag_h)
  if revtag_nsh == old_revtag_nsh:
    notify('revtag.nsh is up to date')
  else:
    notify('updating revtag.nsh')
    open('revtag.nsh', 'w').write(revtag_nsh)
elif sys.argv[1] == '--tag-file':
  get_revnum()
  fdata = open(sys.argv[3], 'r').read()
  for key, value in macro_list:
    fdata = fdata.replace('@%s@' % key, value)
  sys.stdout.write(fdata)
else:
  usage()
