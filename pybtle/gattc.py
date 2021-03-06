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
import cmd
import uuid

SMP_SECURITY_LOW    =  1
SMP_SECURITY_MEDIUM =  2
SMP_SECURITY_HIGH   =  3

GATT_INVALID      = 0x00
GATT_NOTIFICATION = 0x01
GATT_INDICATION   = 0x02

class gattc():

    """Generic Attribute Profile (GATT) Client class

    code example 
    ::
        import gattc
        import cmd
        import gap
        client = gattc.gattc(cmd.cmd())
        client.connect(address, addr_type)
        client.service_discovery()
    """
    def __init__(self, cmd):
        self.cmd = cmd
        self.attrs = None

    def connect(self, devId, addr, addrtype, sec_level):
        """Create a connection
        
        Args:
            devId (int): Adapter index.
            addr (str): BLE address to connect (00:11:22:33:44:55).
            addrtype (str): "public" or "random"
            sec_level(str): security level from ("low", "medium", "high")

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
        """
        ret = self.cmd.connect(devId, addr, addrtype, sec_level, self.service_discovery)
        return ret

    def service_discovery(self, result):
        self.attrs = result

    def get_db(self):
        """Retrieve GATT services, characteristics, descriptors found on GATT server
    
        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
    
        """
        return self.attrs

    def write_cmd(self, devId, chr, handle, value):
        """Perform a write without response
    
        Args:
            devId (int): adapter index.
            chr (str): characteristic uuid.
            handle (int): characteristic value handle.
            value (str): data to write.
    
        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
    
        """
        ret = self.cmd.write_cmd(devId, chr, handle, value)
        return ret

    def write_req(self, devId, chr, handle, value):
        """Perform a write with response
    
        Args:
            devId (int): adapter index.
            chr (str): characteristic uuid.
            handle (int): characteristic value handle.
            value (str): data to write.
    
        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
    
        """
        ret = self.cmd.write_req(devId, chr, handle, value)
        return ret

    def read(self, devId, chr, handle):
        """Perform a a GATT Read Characteristic or Descriptor procedure
    
        Args:
            devId (int): adapter index.
            chr (str): characteristic uuid.
            handle (int): characteristic value handle.

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
                'value': "read value"
            }
    
        """
        ret = self.cmd.read(devId, chr, handle)
        return ret

    def get_characteristic_by_uuid(self, struuid):
        if self.attrs == None:
            return None

        for attr in self.attrs:
            for chr in attr["service"]["characteristics"]:

                if chr["uuid"].startswith(uuid.str_to_uuid128(struuid)):
                    return chr

        return None

    def notif_ind_handler(self, result):
        try:
            cccd_id = result.pop('cccd_id')
            result["chrc_uuid"] = self.ntf_ind_cb[cccd_id]["chrc_uuid"]
            self.ntf_ind_cb[cccd_id]["callback"](result)
        except ValueError:
            '''
            happens when notification/indication are received
            before subscription response
            '''
            pass

    def subscribe_notification(self, devId, chrc_uuid, notification_cb):
        """Subscribing to Notification.

        Args:
            devId (int): adapter index.
            chr (str): characteristic uuid.
            notification_cb (func): Notification callback.

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
    
        """
        ret = {"status": "error", "reason": ("uuid %s not found" % chrc_uuid)}
        chr = self.get_characteristic_by_uuid(chrc_uuid)
        if None == chr:
            return ret

        if hasattr(self, "ntf_ind_cb"):
            for elt in self.ntf_ind_cb:
                if elt["value_handle"] == chr["value_handle"]:
                    ret["reason"] = "subscription already exists"
                    return ret
        else:
            self.ntf_ind_cb = {}

        ret = self.cmd.subscribe(devId, chr["value_handle"],
                                 GATT_NOTIFICATION,
                                 self.notif_ind_handler)
        if ret["status"] == "ok":
            info = {}
            info["value_handle"] = chr["value_handle"]
            info["cccd_id"] = ret["result"]["cccd_id"]
            info["chrc_uuid"] = chrc_uuid
            info["callback"] = notification_cb

            self.ntf_ind_cb[ret["result"]["cccd_id"]] = info

        return ret

    def unsubscribe_notification(self, devId, chrc_uuid):
        """Unsubscribing to Notification.

        Args:
            devId (int): adapter index.
            chr (str): characteristic uuid.

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }
    
        """
        ret = {"status": "error", "reason": ("uuid %s not found" % chrc_uuid)}
        chr = self.get_characteristic_by_uuid(chrc_uuid)
        if None == chr:
            return ret

        ret["reason"] = "none subscription for this characteristic"
        if False == hasattr(self, "ntf_ind_cb"):
            return ret

        for idx in self.ntf_ind_cb:
            elt = self.ntf_ind_cb[idx]
            if elt["value_handle"] == chr["value_handle"]:
                ret = self.cmd.unsubscribe(devId, elt["cccd_id"])
                if ret["status"] == "ok":
                    self.ntf_ind_cb.pop(elt["cccd_id"])
                    break

        return ret
