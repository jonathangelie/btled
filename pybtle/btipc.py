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
import btsocket
import time
import struct
import re
import binascii
import Queue

class btipc:

    IPC_MSG_TYPE_REQ    = 0
    IPC_MSG_TYPE_RSP    = 1
    IPC_MSG_TYPE_EVENT  = 2

    def __init__(self, evt_delegate = None):
        self.socket = btsocket.btsocket(delegate = self.ipc_receive)
        self.mtu = self.socket.getmtu()
        self.q_resp = Queue.Queue()
        self.evt_delegate = evt_delegate

        #print("mtu %d" % self.mtu)

    def ipc_receive(self, data):
        '''
        +--------------------+----------+----------------+
        |          0         |     1    | 2:data_len + 2 |
        +--------------------+----------+----------------+
        |  IPC_MSG_TYPE_RSP  |          |                |
        |         or         | data len |      data      |
        | IPC_MSG_TYPE_EVENT |          |                |
        +--------------------+----------+----------------+
        '''
        (msg_type, len) = struct.unpack("<BB", data[:2])
        content = data[2:]
        msg = { "len" : len, "content": data[2:]}
        if msg_type == self.IPC_MSG_TYPE_RSP:
            self.q_resp.put(msg)
        elif msg_type == self.IPC_MSG_TYPE_EVENT:
            if self.evt_delegate != None:
                self.evt_delegate(msg)

        #print ("ipc_rx type(%d) | len(%d) | %s" %
        #       (msg_type, len, binascii.hexlify(content)))

        self.ipc_loopback_rx(content)

    def ipc_loopback_tx(self, SN = 0):
        msg = "TX(%d)" % SN
        self.ipc_send(5, msg, len(msg))

    def ipc_loopback_rx(self, data):
        s = re.search("TX\((.+)\)", data)
        if s != None:
            SN = int(s.group(1))
            # loopback
            self.ipc_loopback_tx(SN+1)

    def ipc_send_req(self, data, data_len):
        '''
        +------------------+----------+----------------+
        |         0        |     1    | 2:data_len + 2 |
        +------------------+----------+----------------+
        | IPC_MSG_TYPE_REQ | data len |      data      |
        +------------------+----------+----------------+
        '''
        if (data_len + 2) > self.mtu:
            print "message too long"
            return False

        pkt = struct.pack('<BB', self.IPC_MSG_TYPE_REQ, data_len)
        pkt += data

        #print ("ipc_tx type(%d) | len(%d) | %s" % (self.IPC_MSG_TYPE_REQ, data_len, data))
        return self.socket.send(pkt, len(pkt))

    def getmtu(self):
        return self.mtu
