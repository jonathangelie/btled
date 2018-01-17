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
import re
import time
import binascii

# for options parsing 
import textwrap
import argparse

path = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
sys.path.append(os.path.normpath(path + '/../pybtle/'))
cmd = imp.load_source('cmd', os.path.normpath(path + '/../pybtle/cmd.py'))

class adapter:
    def __init__(self, controller = 'hci0'):
        s = re.search("hci(.+)", controller)
        if s != None:
            self.devId = int(s.group(1))
            self.cmd = cmd.cmd()
        else:
            print "unknown controller %s" % controller

    def scan_result(self, result):
        #print result
        print("addr: %s rssi:%d " % (result["addr"],result["rssi"]))

    def read_controller_info(self):
        try :
            print("reading controller information")
            ret = self.cmd.read_controller_info(self.devId)
            if ret["status"] == "ok":
                print ret
                content = ret["result"]

                print("addr: %s" % content["addr"])
                print("version:%d" % content["version"])
                print("manufacturer:%d" % content["manufacturer"])
                print("supported settings = 0x%04X" % content["supported_settings"])
                print("currentsettings settings = 0x%04X" % content["current_settings"])
                print("devclass = 0x%04X" %  content["dev_class"])
                print("name %s" % (content["name"]))

            else:
                print ("error occured while reading controller info" )
        except cmd.cmdException as e:
            print str(e)
            pass

    def power(self, action = "on"):
        try :
            print("powering %s" % action)
            if action in "on":
                ret = self.cmd.power_on(self.devId,)
            elif action in "off":
                ret = self.cmd.power_off(self.devId)
                if ret["status"] != "ok":
                    print ("error occured while powering %s" % action)

        except cmd.cmdException as e:
            print str(e)
            pass

    def scan(self, action = "start"):
        try :
            print("scan %s" % action)
            if action in "start":
                self.cmd.scan_start(self.devId, self.scan_result)
            elif action in "stop":
                self.cmd.scan_stop(self.devId)
        except cmd.cmdException as e:
            print str(e)
            pass

class CustomerFormatter(argparse.ArgumentDefaultsHelpFormatter, argparse.RawTextHelpFormatter):
    pass

parser = argparse.ArgumentParser(formatter_class=CustomerFormatter,
                                 description=textwrap.dedent('''
    Script catching debug message sent by device throw BLE connection
    '''))
parser.add_argument('-c', '--controller', action='store', default='hci0',
                    help='controller')
arg = parser.parse_args(sys.argv[1:])

try:
    device = adapter(arg.controller)
    device.read_controller_info()
    device.power(action ="on")
    device.scan(action = "start")
    time.sleep(10)
    device.scan(action = "stop")
    device.power(action ="off")

except KeyboardInterrupt:
    print("\nscan.py exit")

