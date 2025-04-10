#!/bin/bash

PORT={{ port }}
export FLASK_APP={{ flask_app }}

# run without logs

# run webserver
flask --app "$FLASK_APP" run --host=0.0.0.0 --port="$PORT"  > /dev/null &

# run sensor reader
/home/sveng/scd30/bin/query_scd30 > /dev/null &

exit 0
