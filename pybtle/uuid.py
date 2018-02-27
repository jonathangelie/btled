""""""
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
import struct
import binascii
import re
import sys
import json
import imp
import inspect, os
mylocalpath = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

json_file = mylocalpath + "/uuids.json"

this = sys.modules[__name__]
this.__uuid__ = None

base_uuid = "-0000-1000-8000-00805f9b34fb"

def uuid128_to_str(value):
    """Converts 128bits UUID to its human representation.

    :param value: 128bits UUID.
    :type value: string
    :rtype: string
    """
    if this.__uuid__ == None:
        this.__uuid__ = uuid()
    for u128 in this.__uuid__.instance.uuids["uuid128"]:
        if u128["uuid"] == value:
            return u128["str"]
    return value

def uuid_to_str(value):
    """Converts 16bits or 128bits UUID to its human representation.

    :param value: 16bits or 128bits UUID.
    :type value: string
    :rtype: string
    """
    u128 = value
    u16 = re.search("0x(.+)", value)
    if None != u16:
        u128 = u16.group(1).rjust(8,'0') + base_uuid

    return uuid128_to_str(u128)

def str_to_uuid128(value):
    """Return 128bits UUID from UUUID string name.

    :param value: UUID name.
    :type value: string
    :rtype: string
    """
    if this.__uuid__ == None:
        this.__uuid__ = uuid()
    for u128 in this.__uuid__.instance.uuids["uuid128"]:
        if u128["str"] == value:
            return u128["uuid"]

    # we did our best effort
    return value

class uuid:

    class __impl:
        def __init__(self):
            with open(json_file, 'r') as f:
                self.uuids = json.load(f)

    __instance = None

    def __init__(self):
        if uuid.__instance is None:
            uuid.__instance = uuid.__impl()
        self.instance = uuid.__instance

