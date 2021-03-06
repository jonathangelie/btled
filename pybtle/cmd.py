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

CMD_GATTC_CONNECT_REQ           = 7     # [devid | addr | addrtype | sec_level
CMD_GATTC_WRITE_CMD             = 8     # [devid | handle(u16) | data ]
CMD_GATTC_WRITE_REQ             = 9     # [devid | handle(u16) | data ]
CMD_GATTC_READ_REQ              = 10    # [devid | handle(u16)]
CMD_GATTC_SUBSCRIBE_REQ         = 11    # [devid | handle(u16) | None(0)|Nty(1)|Ind(2)]
CMD_GATTC_UNSUBSCRIBE_REQ       = 12    # [devid | cccd_if(u8)

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

UUID_STR_MAX_LEN         = 36

class cmdException(Exception):
    pass

class cmd :
    """Responsible for (de)serializing command and notify upper layer
    """
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
            self.delegate[EVT_GATTC_DISC_PRIMARY](self.attrs)
            return

        data = data[1:]
        (start, end) = struct.unpack('<HH', data[:4])
        data = data[4:]
        data_len -= 5
        uuid = data[:UUID_STR_MAX_LEN]

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
        (handle, value_handle, ext_prop, properties) = struct.unpack('<HHHB', data[:7])
        data = data[7:]
        data_len -= 7
        uuid = data[:UUID_STR_MAX_LEN]

        chr = {}
        chr["handle"] = handle
        chr["value_handle"] = value_handle
        chr["properties"] = properties
        chr["ext_prop"] = ext_prop
        chr["uuid"] = uuid
        chr["desc"] = []
        self.attr["service"]["characteristics"].append(chr)

    def parse_discover_descriptor_evt(self, data):

        if self.attr and "characteristics" in self.attr["service"]:

            desc = {}
            data_len = struct.unpack('>B', data[:1])[0]
            data = data[1:]

            (desc["handle"], desc["uuid16"]) = struct.unpack('<HH', data[:4])
            data = data[4:]
            data_len -= 5
            desc["uuid"] = data[:UUID_STR_MAX_LEN]

            self.attr["service"]["characteristics"][-1]["desc"].append(desc)

    def parse_notification_evt(self, data):

        data_len = struct.unpack('>B', data[:1])[0]
        data = data[1:]

        notif = {}
        (value_handle, len, cccd_id) = struct.unpack('<HHB', data[:5])
        data = data[5:]

        notif["cccd_id"] = cccd_id
        notif["value_handle"] = value_handle
        notif["data_len"] = len
        notif["data"] = data[:len]
        self.delegate[EVT_GATTC_NOTIFICATION](notif)

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
        elif evt == EVT_GATTC_NOTIFICATION:
            self.parse_notification_evt(data)

    def parse_read_controller_info_rsp(self, data, data_len):
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

    def parse_read_characteristic_rsp(self, data, data_len):
        dict = {}
        dict["length"] = data_len
        dict["value"] = data[:data_len]

        return dict

    def parse_write_characteristic_rsp(self, data, data_len):
        '''
        nothing to do as status has already been checked
        '''

    def parse_subscibe_rsp(self, data, data_len):
        dict = {}
        dict["cccd_id"] = struct.unpack('<B', data[0])[0]

        return dict

    def parse_unsubscibe_rsp(self, data, data_len):
        '''
        nothing to do as status has already been checked
        '''

    def parse_resp(self, adapter, cmd, data):
        ret = {"status": "error", "reason": "unknown command"}

        (status, data_len) = struct.unpack('>BB', data[:2])
        data = data[2:]

        if status == 0:
            ret["status"] = "ok"
            del ret["reason"]

            if cmd == CMD_MGMT_READ_CONTROLLER_INFO:
                ret["result"] = self.parse_read_controller_info_rsp(data, data_len)
            if cmd == CMD_GATTC_READ_REQ:
                ret["result"] = self.parse_read_characteristic_rsp(data, data_len)
            if cmd == CMD_GATTC_WRITE_REQ or cmd == CMD_GATTC_WRITE_CMD:
                ret["result"] = parse_write_characteristic_rsp(data, data_len)
            if cmd == CMD_GATTC_SUBSCRIBE_REQ:
                ret["result"] = self.parse_subscibe_rsp(data, data_len)
            if cmd == CMD_GATTC_UNSUBSCRIBE_REQ:
                ret["result"] = self.parse_unsubscibe_rsp(data, data_len)
        else:
            ret["err_code"] = status
            ret["reason"] = data[:data_len]

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
        print("cmd(%d) response status [%s]" % (cmd, ret["status"]))
        return ret

    def read_controller_info(self, adapter):
        """Sending reading controller information command
        
        Args:
            adapter (int): Adapter index

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
                'version': "hex"
                'manufacturer': "hex"
                'supported_settings': "hex"
                'current_settings': "hex"
                'dev_class': "hex"
                'name': "str"
            }
        """
        bin = struct.pack('>B', 0)
        return self.send_cmd(adapter, CMD_MGMT_READ_CONTROLLER_INFO, bin)

    def conn_param(self, adapter, addr, min, max, latency, timeout):
        """Sending set connection parameter command
        
        Args:
            adapter (int): Adapter index
            addr (str): BLE address
            min (int): Minimum connection interval
            max (int): Maximum connection interval
            latency (int): latency
            timeout (int): Supervision timeout

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
        """
        bin = addr
        bin += struct.pack('>HHHH', min, max, latency, timeout)
        return self.send_cmd(adapter, CMD_MGMT_SET_CONNECTION_PARAM, bin)

    def set_local_name(self, adapter, name):
        """Sending set Advertising local Name command
        
        Args:
            adapter (int): Adapter index
            name (str): Advertising local Name

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
        """
        bin = struct.pack('>B', len(name))
        bin += name
        return self.send_cmd(adapter, CMD_MGMT_SET_LOCAL_NAME, bin)

    def power_on(self, adapter):
        """Sending powering ON command
        
        Args:
            adapter (int): Adapter index

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
        """
        bin = struct.pack('>B', 1)
        return self.send_cmd(adapter, CMD_MGMT_POWER, bin)

    def power_off(self, adapter):
        """Sendind powering OFF command
        
        Args:
            adapter (int): Adapter index

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
        """
        bin = struct.pack('>B', 0)
        return self.send_cmd(adapter, CMD_MGMT_POWER, bin)

    def scan_start(self, adapter, scan_delegate):
        """Sending start BLE scan command
        
        Args:
            adapter (int): Adapter index

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
        """
        self.delegate[EVT_SCAN_RESULT] = scan_delegate
        bin = struct.pack('>B', 1)
        return self.send_cmd(adapter, CMD_MGMT_SCAN, bin)

    def scan_stop(self, adapter):
        """Sending stop BLE scan command
        
        Args:
            adapter (int): Adapter index

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
        """
        bin = struct.pack('>B', 0)
        return self.send_cmd(adapter, CMD_MGMT_SCAN, bin)

    def connect(self, adapter, addr, addrtype, sec_level, service_discovery_cb):
        """Sending create connection command
        
        Args:
            adapter (int): Adapter index.
            addr (str): BLE address to connect (00:11:22:33:44:55).
            addrtype (str): "public" or "random"
            sec_level(str): security level from ("low", "medium", "high")
            discovery_cb (callback) function called on_service_discovery

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }

        """
        if addrtype == "public":
            addrtype = 1
        elif addrtype == "random":
            addrtype = 2
        else:
            ret = {}
            ret["result"] = "error"
            ret["reason"] = "wrong address type"
            return ret

        self.delegate[EVT_GATTC_DISC_PRIMARY] = service_discovery_cb
        bin = addr
        bin = ":".join([addr[x:x+2] for x in range(0,len(addr),3)][::-1])
        bin += struct.pack('>BB', addrtype, sec_level)
        self.reset_db()
        return self.send_cmd(adapter, CMD_GATTC_CONNECT_REQ, bin, timeout = 15)

    def subscribe(self, adapter, value_handle, value, notification_cb):
        """Sending Notification / Indication subscription command

        Args:
            adapter (int): adapter index.
            value_handle (int): characteristic value handle
            value (int): 1 for Notification; 2 for Indication
            notification_cb (func): Notification callback.

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }

        """
        if value != 0:
            self.delegate[EVT_GATTC_NOTIFICATION] = notification_cb
        bin = struct.pack('>BH', value, value_handle)
        return self.send_cmd(adapter, CMD_GATTC_SUBSCRIBE_REQ, bin)

    def unsubscribe(self, adapter, cccd_id):
        """Sending stop BLE scan command
        
        Args:
            adapter (int): Adapter index
            cccd_id (int): Subscription ID

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
        """
        bin = struct.pack('>B', cccd_id)
        return self.send_cmd(adapter, CMD_GATTC_UNSUBSCRIBE_REQ, bin)
