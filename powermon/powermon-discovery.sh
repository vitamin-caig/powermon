#!/bin/bash

echo -ne "{\n  \"data\":\n  [\n"
DELIMITER=
for dev in /dev/ttyUSB*
do
  powermon ${dev} test > /dev/null && echo -ne "    ${DELIMITER}{\"{#POWERMONDEVICE}\": \"${dev}\"}\n"
  DELIMITER=","
done
echo -ne "  ]\n}\n"
