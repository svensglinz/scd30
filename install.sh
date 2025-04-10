#!/bin/bash

FLASK_APP="/home/sveng/scd30/src/data_backend.py"
PORT="8080"
READ_INTERVAL=30
WORKING_DIR="$(dirname ${BASH_SOURCE[0]})"

cd "$WORKING_DIR"

# check sudo
if [ "$(id -u)" -ne 0 ]; then
	echo "please run script as sudo"
	exit 1
fi

check_error(){
	if [ $? -ne 0 ]; then
		echo "$1"
		exit 1
	fi
}

unroll(){
	exit 1
}

trap unroll ERR

echo "installing scd30 temperature server"
echo "***************"

# check dependencies
#check_dependency "python3"
#check_dependendy "make"
#check_dependency "..."

# compile binaries
# make

# substitute variables into executable
sed "s|{{ port }}|$PORT|g; s|{{ flask_app }}|$FLASK_APP|g; s|{{ bin_path }}|$BIN_PATH|g; s|{{ app_dir }}|$PWD|g" ./scripts/scd30-start-tmp.sh > ./scripts/scd30-start.sh
sed "s|{{ app_dir }}|$PWD|g" ./service/scd30-tmp.service > ./service/scd30.service

chmod +x ./scripts/scd30-start.sh

# install systemd service & run

echo "copying ./scripts/scd30.service to /etc/sytemd/system/"
cp ./service/scd30.service /etc/systemd/system/
check_error "could not move scd30.service to /etc/systemd/system"

systemctl daemon-reload
# systemctl start scd30.service
check_error "error starting scd30 service"

echo "installation successful"
# echo "start the server with systemctl start scd30"
echo "*************"
exit 0
