# IQRF GW daemon repository

http://repos.iqrfsdk.org/

- iqrf-daemon_0.0.1_amd64.deb 
- libpaho.mqtt.c_1.1.0_amd64.deb 

## Download public key to verify the packages from the repository

```Bash
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 07BABA62
```

## Add repository to the source list

```Bash
sudo add-apt-repository "deb http://repos.iqrfsdk.org xenial testing"
sudo apt-get update
```

## Installthe IQRF daemon

```Bash
sudo apt-get install iqrf-daemon
```

## Check the daemon service status

```Bash
sudo systemctl status iqrf-daemon.service

● iqrf-daemon.service - IQRF daemon iqrf_startup
   Loaded: loaded (/lib/systemd/system/iqrf-daemon.service; enabled; vendor preset: enabled)
   Active: active (running) since Čt 2017-03-30 23:46:50 CEST; 2min 58s ago
 Main PID: 9540 (iqrf_startup)
   CGroup: /system.slice/iqrf-daemon.service
           └─9540 /usr/bin/iqrf_startup /etc/iqrf-daemon/config.json 
```

Configure components JSON file and restart the service.

```Bash
sudo systemctl restart iqrf-daemon.service
```

## Content of the iqrf-daemon (v0.0.1) package 

```Bash
dpkg -L iqrf-daemon

/.
/usr
/usr/bin
/usr/bin/iqrf_startup
/usr/share
/usr/share/doc
/usr/share/doc/iqrf-daemon
/usr/share/doc/iqrf-daemon/copyright
/usr/share/doc/iqrf-daemon/changelog.gz
/lib
/lib/systemd
/lib/systemd/system
/lib/systemd/system/iqrf-daemon.service
/etc
/etc/iqrf-daemon
/etc/iqrf-daemon/config.json
/etc/iqrf-daemon/MqMessaging.json
/etc/iqrf-daemon/TracerFile.json
/etc/iqrf-daemon/JsonSerializer.json
/etc/iqrf-daemon/IqrfInterface.json
/etc/iqrf-daemon/SimpleSerializer.json
/etc/iqrf-daemon/MqttMessaging.json
/etc/iqrf-daemon/Scheduler.json
/etc/iqrf-daemon/ClientServicePlain.json
```

## Content of the libpaho.mqtt.c (v1.1.0) package 

```Bash
dpkg -L libpaho.mqtt.c

/.
/usr
/usr/lib
/usr/lib/libpaho-mqtt3c.so.1.1.0
/usr/lib/libpaho-mqtt3as.so.1.1.0
/usr/lib/libpaho-mqtt3cs.so.1.1.0
/usr/lib/libpaho-mqtt3a.so.1.1.0
/usr/bin
/usr/bin/MQTTVersion
/usr/share
/usr/share/doc
/usr/share/doc/libpaho.mqtt.c
/usr/share/doc/libpaho.mqtt.c/copyright
/usr/share/doc/libpaho.mqtt.c/changelog.gz 
```