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
import os

class gap():

    """Generic Access Profile (GAP) class
    """
    def __init__(self, cmd):
        self.cmd = cmd
        self.pcp = {}

    def connect(self, devId, addr, addrtype, sec_level, discovery_cb = None):
        """Create a connection
        
        Args:
            devId (int): Adapter index.
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
        if any(self.pcp) :
            self.cmd.conn_params(devId, addr, addrtype, 
                             self.pcp["min"], self.pcp["max"], 
                             self.pcp["latency"], self.pcp["timeout"])
        ret = self.cmd.connect(devId, addr, addrtype, sec_level, discovery_cb)
        return ret

    def connection_setpcp(self, min_interval, max_interval, latency, timeout):
        """Set GAP Peripheral Preferred Connection Parameters
        
        Args:
            min_interval (int): Minimum connection interval
            max_interval (int): Maximum connection interval
            latency (int): latency
            timeout (int): Supervision timeout

        Returns:
        ::
            {
                'result': ("ok", "error"),
                'reason': "failure reason"
            }

        """
        self.pcp["min"] = min_interval
        self.pcp["max"] = max_interval
        self.pcp["latency"] = latency
        self.pcp["timeout"] = timeout

    def advertising_set_interval(self, min_interval, max_interval):
        """Set Advertisinig interval
        
        Args:
            min_interval (int): Minimum advertising interval (> 20ms)
            max_interval (int): Maximum advertising interval (< 10240ms)

        """
        if min_interval < 20 :
            min_interval = 20

        if max_interval > 10240 :
            max_interval = 10240

        min = (1000 * min_interval) / 625
        max = (1000 * max_interval) / 625

        cmd = ("sudo bash -c \"echo %d > /sys/kernel/debug/bluetooth/%s/adv_min_interval\"" %
                  (min, self._iface))
        os.system(cmd)

        cmd = ("sudo bash -c \"echo %d > /sys/kernel/debug/bluetooth/%s/adv_max_interval\"" %
                  (max, self._iface))
        os.system(cmd)
        
    def advertising_set_local_name(self, name):
        """Set Advertisinig Local Name
        
        Args:
            Name (str): Local Name

        """