# Bluetooth Low Energy Daemon - btled

## Motivation

The origin of the btled server/client application come from a personnal desire to gathers 
into one interface most of powerful capabilities offers by Linux bluetooth stack.

In fact Bluez exposed many useful tools like btmgmt, btmon, btgatt-client, btattach, but while developing, verifying or testing BLE firmware, we have to jump from a binary to an other one, as their use are different.

Fortunately, exists two majors python script for automate our hardware testing:

* pyGatt is a awesome mulitplatform (Linux and Windows) scripts.
However, as pyGatt is based on the deprecated gatttool, it does not offers mgmt capacity like setting local name, connection parameters for instance.
* Bluepy is another tools communicating (through stdin, stdout) with its corresponding bluepy-helper binary to handle Bluez management interface.
However, the main drawback is that Bluepy is launching as many bluepy-helper binary as BT interface you want to work with.

## Design

Btled component is inspired by a whole open source project, but is written from scratch focusing on speed, simplicity, and flexibility.

![Btled design](./images/btled_design.png)
### Build

```shell
make -C src
```

### Installation

```shell
sudo apt-get install ./dpk/btled_1.0_all.deb
```

### Example

```shell
# launching Bluetooth Low Energy daemon
$ sudo /etc/init.d/btled --start
09:39:55 main [I] Starting Bluetooth Low Energy daemon
```

```shell
# running scan example
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
