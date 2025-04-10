from RPLCD.i2c import CharLCD
import time
import requests
# Change '0x3F' to your detected address (0x27 or 0x3F)

url = 'http://192.168.1.193:8080'

lcd = CharLCD(i2c_expander='PCF8574', address=0x27, port=1,
              cols=16, rows=2,  auto_linebreaks=True)

while True:
	lcd.clear()
	resp = requests.get(url).json()

	co2 = resp['co2']
	temp = resp['temp']
	humidity = resp['humidity']

	lcd.write_string(f"T:{round(temp, 1)} / H:{round(humidity, 1)} Co2 : {round(co2, 0)}")
	time.sleep(5)
