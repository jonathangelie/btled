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
import socket
import threading
import select
import struct

class btsocket:
    socket_addr = "/var/run/btled"

    def __init__(self, delegate = None):
        self.client_sock = None
        self.delegate = delegate
        self.mtu = None
        self.rx = ""
        self.open()
        self.connect()
        self.mtu_negociation(timeout = 1)

        if self.mtu and self.client_sock:
            # creating thread for polling socket rx message
            self.thread = threading.Thread(target = self.receive_thread, name="rx_socket")
            self.thread.daemon = True
            self.thread.start()

    def mtu_negociation(self ,timeout = 1):
        readable, writable, exceptional = select.select([self.client_sock],
                                                        [], [], timeout)

        if len(readable) :
            pkt = self.client_sock.recv(2)
            (h, l) = struct.unpack("<BB", pkt[:2])
            self.mtu = 0xff & (h << 8)
            self.mtu += 0xff & l
            #print ("mtu = %d" % self.mtu)

    def getmtu(self):
        return self.mtu

    def withDelegate(self, delegate):
        self.delegate = delegate

    def open(self):
        self.client_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.client_sock.setblocking(0)

    def connect(self):
        ret = False
        try:
            self.client_sock.connect(self.socket_addr)
            ret = True
        except socket.error as e:
            print("socket connection failed " + str(e))
            print("please run with root access")
            raise

        return ret

    def send(self, data, data_len):
        if self.mtu == None:
            return False

        if data_len > self.mtu:
            return False

        pkt = data.ljust(self.mtu, '\0')
        ret = False
        bytecnt = 0;
        while bytecnt < self.mtu:
            try:
                
                sent = self.client_sock.send(pkt[bytecnt:])
                if sent == 0:
                    print ("nothing sent - TX error")
                    break;
                bytecnt += sent
            except socket.error as e:
                print("socket connection failed " + str(e))
                pass
                break

        if (bytecnt >= self.mtu):
            ret = True

        return ret

    def receive_thread(self):
        timeout = 1
        while True:
            readable, writable, exceptional = select.select([self.client_sock],
                                                            [], [], timeout)
    
            if len(readable) :
                data_rcv = self.client_sock.recv(self.mtu)
                self.rx += data_rcv

                if len(self.rx) >= self.mtu:
                    if self.delegate:
                        self.delegate(self.rx[:self.mtu])

                    self.rx = self.rx[self.mtu:]

    def close(self):
        if self.client_sock :
            self.client_sock.close()
            self.thread.join(1)
            self.client_sock = None
            self.mtu = None
