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
import btipc
import struct
import time
import binascii
import Queue


CMD_MGMT_GET_DEVICE_INFO        = 0     # [devid]
CMD_MGMT_RESET                  = 1     # [devid]
CMD_MGMT_POWER                  = 2     # [devid] (mode(u8)=0:off, 1:on)
CMD_MGMT_SET_LOCAL_NAME         = 3     # [devid | len(u8) < 31bytes | data]
CMD_MGMT_SET_CONNECTION_PARAM   = 4     # [devid | addr | addrtype | min(u8) | max(u8)| interval(u8) | timeout(u16)]
CMD_MGMT_SCAN                   = 5     # [devid | (mode(u8)=0:stop, 1:start) | timeout_ms(u16)]
CMD_MGMT_READ_CONTROLLER_INFO   = 6     # [devid

CMD_GATTC_CONNECT_REQ           = 7    # [devid | addr | addrtype | sec_level
CMD_GATTC_WRITE_CMD             = 8     # [devid | handle(u16) | data ]
CMD_GATTC_WRITE_REQ             = 9     # [devid | handle(u16) | data ]
CMD_GATTC_READ_REQ              = 10     # [devid | handle(u16)]

EVT_CONNECTED            = 0
EVT_DISCONNECTED         = 1
EVT_SCAN_STATUS          = 2
EVT_SCAN_RESULT          = 3
EVT_NEW_CONN_PARAM       = 4
EVT_GATTC_NOTIFICATION   = 5
EVT_GATTC_INDICATION     = 6
EVT_GATTC_DISC_PRIMARY   = 7
EVT_GATTC_DISC_CHAR      = 8
EVT_GATTC_DISC_DESC      = 9

class cmdException(Exception):
    pass

class cmd :

    def __init__(self):
        self.ipc = btipc.btipc(evt_delegate = self.parse_event)
        self.cmd = None
        self.delegate = {}

    def reset_db(self):
        self.attrs = []
        self.attr = None

    def parse_scan_result_evt(self, data):
        dict = {}
        data_len = struct.unpack('>B', data[:1])[0]
        data = data[1:]
        dict["flags"] = struct.unpack('<L', data[:4])[0]
        data = data[4:]
        litle_addr = ''.join('%02x' % ord(b) for b in data[:6])
        dict["addr"] = ":".join([litle_addr[x:x+2] for x in range(0,len(litle_addr),2)][::-1])
        data = data[6:]
        (dict["addr_type"], dict["rssi"], dict["adv_data_len"]) = struct.unpack('<BbB', data[:3])
        data = data[3:]
        dict["adv_data"] = data[: dict["adv_data_len"]]
        try:
            self.delegate[EVT_SCAN_RESULT](dict)
        except KeyError:
            pass

    def parse_discover_primary_evt(self, data):

        if None != self.attr:
            self.attrs.append(self.attr)

        data_len = struct.unpack('>B', data[:1])[0]
        if data_len == 0:
            self.delegate[EVT_GATTC_DISC_PRIMARY](EVT_GATTC_DISC_PRIMARY, self.attrs)
            return

        data = data[1:]
        (start, end) = struct.unpack('<HH', data[:4])
        data = data[4:]
        data_len -= 5
        uuid = data[:data_len]

        self.attr = {}
        self.attr["service"] = {}
        self.attr["service"]["start"] = start
        self.attr["service"]["end"] = end
        self.attr["service"]["uuid"] = uuid
        self.attr["service"]["characteristics"] = []
        
    def parse_discover_characteristc_evt(self, data):

        if None == self.attr:
            return

        data_len = struct.unpack('>B', data[:1])[0]

        data = data[1:]
        (handle, value_handle, properties, ext_prop) = struct.unpack('<HHBH', data[:7])
        data = data[7:]
        data_len -= 8
        uuid = data[:data_len]

        chr = {}
        chr["handle"] = handle
        chr["value_handle"] = value_handle
        chr["properties"] = properties
        chr["ext_prop"] = ext_prop
        chr["uuid"] = uuid
        chr["desc"] = []
        self.attr["service"]["characteristics"].append(chr)

    def parse_discover_descriptor_evt(self, data):

        if self.attr and hasattr(self.attr["service"], "characteristics"):

            desc = {}
            data_len = struct.unpack('>B', data[:1])[0]

            data = data[1:]
            (desc["handle"], desc["uuid16"]) = struct.unpack('<HH', data[:4])
            data = data[4:]
            data_len -= 5
            desc["uuid"] = data[:data_len]

            self.attr["service"]["characteristics"][-1]["desc"].append(desc)

    def parse_event(self, evt_dict):
        (adapter, evt, status) = struct.unpack('>BBB', evt_dict["content"][:3])
        data = evt_dict["content"][3:]
        if evt == EVT_SCAN_RESULT:
            self.parse_scan_result_evt(data)
        elif evt == EVT_GATTC_DISC_PRIMARY:
            self.parse_discover_primary_evt(data)
        elif evt == EVT_GATTC_DISC_CHAR:
            self.parse_discover_characteristc_evt(data)
        elif evt == EVT_GATTC_DISC_DESC:
            self.parse_discover_descriptor_evt(data)

    def parse_read_controller_info(self, data, data_len):
        dict = {}
        litle_addr = ''.join('%02x' % ord(b) for b in data[:6])
        dict["addr"] = ":".join([litle_addr[x:x+2] for x in range(0,len(litle_addr),2)][::-1])
        data = data[6:]
        data_len -= 6
        dict["version"] = struct.unpack('<B', data[:1])[0]
        data = data[1:]
        data_len -= 1
        dict["manufacturer"] = struct.unpack('<H', data[:2])[0]
        data = data[2:]
        data_len -= 2
        dict["supported_settings"], dict["current_settings"] = struct.unpack('<LL', data[:8])
        data =data[8:]
        data_len -= 8
        dict["dev_class"] = binascii.hexlify(bytearray(data[:3]))
        dict["dev_class"] = (struct.unpack('<B', data[0])[0] |
                             (struct.unpack('<B', data[1])[0] << 8) |
                             (struct.unpack('<B', data[0])[0] << 16));
        data = data[3:]
        data_len -= 3

        end = data.find('\0', 0)
        if end != -1:
            data_len = end

        dict["name"] = data[:data_len]

        return dict

    def parse_resp(self, adapter, cmd, data):
       ret = {"status": "error", "reason": "unknown command"}

       status = struct.unpack('>B', data[:1])[0]
       if status == 0:
           data = data[1:]
           ret["status"] = "ok"
           del ret["reason"]
           data_len = struct.unpack('>B', data[:1])[0]
           data = data[1:]
           if cmd == CMD_MGMT_READ_CONTROLLER_INFO:
               ret["result"] = self.parse_read_controller_info(data, data_len)
       else:
           ret["reason"] = data[2:]

       return ret

    def wait_resp(self, timeout = 10):
        ret = {"status": "error", "reason": "timeout"}
        try:
            dict = self.ipc.q_resp.get(True, timeout)
            (adapter, cmd) = struct.unpack('>BB', dict["content"][:2])
            if cmd != self.cmd:
                self.ipc.q_resp.put(dict)
                ret["reason"] = "wrong response received"
                return ret
            else :
                ret = self.parse_resp(adapter, cmd, dict["content"][2:])
                return ret
        except Queue.Empty:
            raise cmdException("Command response reception timed out")
        return error

    def send_cmd(self, adapter, cmd, bin = None, timeout = 5 ):
        self.cmd = cmd
        pkt = struct.pack('>BB', adapter, cmd)
        if bin != None :
            pkt += bin
        # print binascii.hexlify(pkt)
        self.ipc.ipc_send_req(pkt, len(pkt))
        ret = self.wait_resp(timeout)
        print("response status [%s]" % ret["status"])
        return ret

    def read_controller_info(self, adapter):
        bin = struct.pack('>B', 0)
        return self.send_cmd(adapter, CMD_MGMT_READ_CONTROLLER_INFO, bin)

    def conn_param(self, adapter, addr, min, max, latency, timeout):
        bin = addr
        bin += struct.pack('>HHHH', min, max, latency, timeout)
        return self.send_cmd(adapter, CMD_MGMT_SET_CONNECTION_PARAM, bin)

    def set_local_name(self, adapter, name):
        bin = struct.pack('>B', len(name))
        bin += name
        return self.send_cmd(adapter, CMD_MGMT_SET_LOCAL_NAME, bin)

    def power_on(self, adapter):
        bin = struct.pack('>B', 1)
        return self.send_cmd(adapter, CMD_MGMT_POWER, bin)

    def power_off(self, adapter):
        bin = struct.pack('>B', 0)
        return self.send_cmd(adapter, CMD_MGMT_POWER, bin)

    def scan_start(self, adapter, scan_delegate):
        self.delegate[EVT_SCAN_RESULT] = scan_delegate
        bin = struct.pack('>B', 1)
        return self.send_cmd(adapter, CMD_MGMT_SCAN, bin)

    def scan_stop(self, adapter):
        bin = struct.pack('>B', 0)
        return self.send_cmd(adapter, CMD_MGMT_SCAN, bin)

    def connect(self, adapter, addr, addrtype, sec_level, service_discovery):
        self.delegate[EVT_GATTC_DISC_PRIMARY] = service_discovery
        bin = addr
        bin = ":".join([addr[x:x+2] for x in range(0,len(addr),3)][::-1])
        bin += struct.pack('>BB', addrtype, sec_level)
        self.reset_db()
        return self.send_cmd(adapter, CMD_GATTC_CONNECT_REQ, bin, timeout = 15)
