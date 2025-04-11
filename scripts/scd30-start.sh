#!/bin/bash

PORT=8080
export FLASK_APP=/home/sveng/scd30/src/data_backend.py

# run without logs

# run webserver
flask --app "$FLASK_APP" run --host=0.0.0.0 --port="$PORT" &

# run sensor reader
/home/sveng/scd30/bin/query_scd30 &

exit 0
