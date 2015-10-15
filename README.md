# tmpchckr (temperature checker)

A little Arduino software made for fun, and to get acquaintanced with Arduino environment.

## What does it do?

It obtains temperature and light reading from sensors (LM35 and photoresistor), and displays it on LCD screen.

Moreover, every now and then, those readings are stored in EEPROM. Right now that happens on three occasions:

- at the beginning,
- when light value changes for more than 35% since last reading,
- after 30 minutes.

Upon every save, timer is reset.

## Schema

To be added later.

## Quirks

Since EEPROM holds just 1024 bytes (and unsigned byte has just 0-255 values), I'm storing temperature in centigrades multiplied by 10 (so 23.5"C becomes 235). Moreover, a hundred is substracted from that (`235 - 100 = 135`), so that there's room for temperatures over 25"C.