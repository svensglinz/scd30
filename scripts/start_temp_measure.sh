#! /bin/bash

SERV_ADDR="192.168.1.193:8080"
PY_VENV='/home/sveng/.py_venv/bin/activate'
ERR_LOG='./error.txt'
PID_LOG='./.started_pid'

clean_up() {
	./stop_temp_measure.sh
}

check_up() {
	ps -p $1 >/dev/null || return 1
}

# check if any processes are still running 
if [ -f $PID_LOG ] && [ -s $PID_LOG ]; then 
	echo "please run ./stop_temp_measure first as some processes are still running"
	exit 1
fi

echo 'starting scd30 sensor'
nohup ./query_scd30 > /dev/null 2>>$ERR_LOG &
SCD30_PID=$!

check_up $SCD30_PID
if [ $? -eq 0 ]; then
	echo "scd30 started successsfully"
	echo $SCD30_PID >> $PID_LOG
else 
	echo "error starting scd30. Aborting"
	clean_up
	exit 1
fi

echo
echo "starting backend server at $SERV_ADDR"
nohup flask --app ./data_backend run --host=0.0.0.0 --port=8080  > /dev/null 2>>$ERR_LOG &
BACKEND_PID=$!

check_up $BACKEND_PID
if [ $? -eq 0 ]; then 
	echo "backend successfully started"
	echo $BACKEND_PID >> $PID_LOG
else
	echo "error starting backend"
	clean_up
	exit 1
fi

echo
echo 'starting LCD display controller'
source $PY_VENV && nohup python3 ./lcd.py > /dev/null 2>>$ERR_LOG &
DISPLAY_PID=$!

check_up $DISPLAY_PID
if [ $? -eq 0 ]; then 
	echo "display controller successfully started"
	echo $DISPLAY_PID >> $PID_LOG
else 
	echo "error starting display controller"
	clean_up
	exit 1
fi 

echo ""
echo "all services started"
echo "run ./stop_temp_measure to stop"
