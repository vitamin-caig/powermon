all: powermon/cpp/powermon

install: 
	install -D powermon/cpp/powermon $(DESTDIR)/usr/local/bin/powermon
	install -D powermon/powermon-discovery.sh $(DESTDIR)/usr/local/bin/powermon-discovery.sh
	install -D zabbix/powermon.conf $(DESTDIR)/etc/zabbix/zabbix_agentd.conf.d/powermon.conf

powermon/cpp/powermon:
	${MAKE} -C powermon/cpp
