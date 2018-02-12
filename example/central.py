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
import imp
import sys
import inspect, os
import time
import re

# for options parsing 
import textwrap
import argparse

path = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
sys.path.append(os.path.normpath(path + '/../pybtle/'))
cmd = imp.load_source('cmd', os.path.normpath(path + '/../pybtle/cmd.py'))
gattc = imp.load_source('gattc', os.path.normpath(path + '/../pybtle/gattc.py'))


c_green     = '\033[32m'
c_yellow    = '\033[33m'
c_white     = '\033[37m'
c_cyan      = '\033[36m'
c_off       = '\033[0m'

class Central:
    def __init__(self, controller = 'hci0'):
        s = re.search("hci(.+)", controller)
        if s != None:
            self.devId = int(s.group(1))
            self.cmd = cmd.cmd()
            self.gattc = gattc.gattc(self.cmd)
        else:
            print "unknown controller %s" % controller

    def start(self):
        ret = {}
        ret["status"] = "error"
        try :
            ret = self.cmd.power_on(self.devId)
            if ret["status"] != "ok":
                ret = self.cmd.read_controller_info(self.devId)

        except cmd.cmdException as e:
            print str(e)
            pass

        return ret

    def stop(self):
        ret = {}
        ret["status"] = "error"
        try :
            ret = self.cmd.power_off(self.devId)
        except cmd.cmdException as e:
            print str(e)
            pass
        return ret

    def connect(self, addr, addrtype, sec_level = gattc.SMP_SECURITY_LOW):
        ret = self.gattc.connect(self.devId, addr, addrtype, sec_level)

    def print_attr(self, attr):
        print c_cyan + "service: " + attr["service"]["uuid"]
        print c_white + "start: %d" % attr["service"]["start"]
        print c_white + "end: %d" % attr["service"]["end"]
        for chr in attr["service"]["characteristics"]:
            print "\t" + c_green + "characteristics: " + chr["uuid"]
            print "\t" + c_white + "handle: %d" % chr["handle"]
            print "\t" + c_white + "value_handle:  %02x" % chr["value_handle"]
            print "\t" + c_white + "properties: %02x" % chr["properties"]
            print "\t" + c_white + "ext_prop: %04x" % chr["ext_prop"]
            for desc in  chr["desc"]:
                print "\t\t" + c_yellow + "descriptor: " + desc["uuid"]
                print "\t\t" + c_white + "handle: " + desc["handle"]

    def service_discovery(self):
        db = self.gattc.get_db()
        if db != None :
            for attr in db:
                self.print_attr(attr)
        return db


class CustomerFormatter(argparse.ArgumentDefaultsHelpFormatter, argparse.RawTextHelpFormatter):
    pass

parser = argparse.ArgumentParser(formatter_class=CustomerFormatter,
                                 description=textwrap.dedent('''
    BTLE Gattc
    '''))
parser.add_argument('-c', '--controller', action='store', default = 'hci0',
                    help='controller')
parser.add_argument('-a', '--address', action='store', default = None,
                    help='gatts address')
parser.add_argument('-d', '--duration', action='store', type=int, default=30,
                    help='test duration ')
parser.add_argument('-t', '--type', action='store', type=int, default=0,
                    help='gatts address type')
arg = parser.parse_args(sys.argv[1:])

start = time.time()

try:
    central = Central(arg.controller)
    central.start()
    central.connect(arg.address, arg.type)

    while (time.time() - start) <= arg.duration :
        ret = central.service_discovery()
        if ret != None:
            break
        time.sleep(1)

except KeyboardInterrupt:
    print("\ncentral.py exit")