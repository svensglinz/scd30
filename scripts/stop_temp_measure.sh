#! /bin/bash 

PID_LOG='./.started_pid'

kill $(cat $PID_LOG)

# clear file
> $PID_LOG
