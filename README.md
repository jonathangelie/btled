# Bluetooth Low Energy Daemon - btled

## Motivation

The origin of the btled server/client application comes from a personnal desire to gathers 
into one interface most of powerful features offers by Bluez stack.

BTled is based on the new D-bus GATT API.

## Design

BTled component is inspired by a whole open source project, but is written focusing on speed, simplicity, and flexibility.

![Btled design](./images/btled_design.png)
### Build

```shell
make -C src
```

### Installation

```shell
sudo apt-get install ./dpk/btled_1.0_all.deb
```

### API
GAP and GATT Client / Server API can be found [here](http://jonathangelie.com/btled/index.html).

### Example

```shell
# launching Bluetooth Low Energy daemon
$ sudo /etc/init.d/btled --start
09:39:55 main [I] Starting Bluetooth Low Energy daemon
```

```shell
# running scan.py example
$ sudo python ./example/scan.py
reading controller information
response status [ok]
{'status': 'ok', 'result': {'supported_settings': 49151, 'addr': '00:1a:7d:da:71:11', 'name': 'Latitude-E5470', 'version': 6, 'dev_class': 0, 'current_settings': 2576, 'manufacturer': 10}}
addr: 00:1a:7d:da:71:11
version:6
manufacturer:10
supported settings = 0xBFFF
currentsettings settings = 0x0A10
devclass = 0x0000
name Latitude-E5470
powering on
response status [ok]
scan start
new event
response status [ok]
new event
addr: f4:f5:d8:67:cd:f5 rssi:-88 
new event
addr: f4:f5:d8:67:cd:f5 rssi:-86 
new event
addr: f4:f5:d8:67:cd:f5 rssi:-91 
new event
addr: f4:f5:d8:67:cd:f5 rssi:-86 
new event
addr: f4:f5:d8:67:cd:f5 rssi:-87 
scan stop
new event
response status [ok]
powering off
response status [ok]

```

```shell
# running central.py example
$ sudo python ./example/central.py -c hci0 -a "00:11:22:33:44:55"
connected with: 00:11:22:33:44:55

service: Device Information
start: 1
end: 8
        characteristic: Model Number String
        handle: 2
        value_handle:  3
        properties: 02
        ext prop: 0000
        characteristic: Serial Number String
        handle: 4
        value_handle:  5
        properties: 02
        ext prop: 0000
        characteristic: Battery Level
        handle: 6
        value_handle:  7
        properties: 12
        ext prop: 0000
service: de305d54-75b4-431b-adb2-eb6b9e546015
start: 9
end: 12
        characteristic: de305d54-75b4-431b-adb2-eb6b9e546016
        handle: 10
        value_handle:  11
        properties: 3a
        ext prop: 0000

Subscribing to characteristic: Battery Level

Notification from characteristic: Battery Level
        value_handle: 7
        len: 2
        data: 67
Notification from characteristic: Battery Level
        value_handle: 7
        len: 2
        data: 66

Unubscribing to characteristic: Battery Level

```

### Running the test

```shell
# launching Bluetooth Low Energy daemon
sudo /etc/init.d/btled --start
```

```shell
# running loopback test
sudo python ./test/ipc_loopback.py
```
## Coding style
[Linux kernel coding sytle](https://www.kernel.org/doc/html/v4.10/process/coding-style.html).

## License

GPL-3.0 - <https://opensource.org/licenses/GPL-3.0>

## Author
Jonathan Gelie <[contact@jonathangelie.com](mailto:contact@jonathangelie.com)>
