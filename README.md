Simple integration of PZEM-004t - based device with Zabbix monitoring tool.

1) Checkout repo to machine with zabbix agent installed
2) `make` to build all the required binaries
3) `sudo make install` to deploy all the required binaries and configs
4) `sudo systemctl restart zabbix-agent` to apply changes

Import required discovery and items from zabbix/template.xml
