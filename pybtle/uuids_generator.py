'''
  Copyright (C) 2018  Jonathan Gelie <contact@jonathangelie.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

import sys
import re
import json

# for options parsing 
import textwrap
import argparse

import imp
import inspect, os
mylocalpath = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

output_file = mylocalpath + "/uuids.json"

class uuids_parser:

    def __init__(self, arg):
        self.fd = None
        self.lines = None
        self.output_file = arg.output
        self.open(arg.input)

    def open(self, filename):
        try:
            with open(filename, "r") as self.fd:
                self.lines = self.fd.readlines()
        except Exception as e:
            print("[ERROR] opening file %s err(%s)" % (filename, str(e)))

    def read(self):
        if self.fd == None:
            return
        self.lines = self.fd.readlines()

    def get_uuids_str(self):
        if self.lines == None:
            return
        start = None
        self.uuid128 = []

        for line in self.lines:
            line = line[:-1]

            if None == re.search("\{(.+)\},", line):
                if re.search("\{ \"(.+)", line):
                    start = line
                    continue

            if None != start:
                line = start + " " + line.replace('\t', '')

            s = re.search("\{ (\"?.+\"?), \"(.+)\"(\\t*)\},", line)
            if None != s:
                dict = {}
                uuid = s.group(1).replace('\"', '')
                if ',' in uuid:
                    continue
                uuid16 = re.search("0x(.+)", uuid)
                if None != uuid16:
                    dict["uuid"] = uuid16.group(1).rjust(8,'0') + "-0000-1000-8000-00805f9b34fb"
                else:
                    dict["uuid"] = uuid
                dict["str"] = s.group(2)
                self.uuid128.append(dict)

            start = None

    def save_to_json(self):
        with open(self.output_file, 'w') as fp:
            d = {}
            d["uuid128"] = self.uuid128
            json.dump(d, fp, indent=4)

    def close(self):
        if self.fd == None:
            return

        self.fd.close()

class CustomerFormatter(argparse.ArgumentDefaultsHelpFormatter, argparse.RawTextHelpFormatter):
    pass

parser = argparse.ArgumentParser(formatter_class=CustomerFormatter,
                                 description=textwrap.dedent('''
    UUIDs generator
    '''))
parser.add_argument('-i', '--input', action='store', default = 'hci0',
                    help='Input file')
parser.add_argument('-o', '--output', action='store', default = output_file,
                    help='output file')
parser.add_argument('-v','--verbose', action='store_true', default = False,
                    help='Increase output verbosity')
arg = parser.parse_args(sys.argv[1:])

try:
    p = uuids_parser(arg)
    p.get_uuids_str()
    if arg.verbose:
        for elt in p.uuid128:
            print elt

    p.save_to_json()
    p.close()

except KeyboardInterrupt:
    print("\exit")
