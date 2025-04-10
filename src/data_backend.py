#! /usr/bin/python3
from flask import Flask
from flask_cors import CORS
import struct 

app = Flask(__name__)
CORS(app)

class SCD30:
    SCD3_DATA = '/dev/shm/sensor_data'
    MAX_CO2 = 1000
    
    @classmethod
    def get_measurements(cls):
        with open(cls.SCD3_DATA, 'rb') as f:
            data = f.read(12)
            vals = struct.unpack('fff', data)
        return({'temp' : vals[1], 'humidity' : vals[2], 'co2' : vals[0]})


# read Co2, Temperature, Humidity from shared memory
@app.route('/')
def get_scd30():
    return SCD30.get_measurements()
