#!/bin/bash
run_image(){
	echo 'running the rovtio docker image'
	docker run -it \
		--rm --gpus all\
		--net="host" \
		--name rovtio \
		--env="DISPLAY" \
		--env="QT_X11_NO_MITSHM=1" \
		ros:rovtio 
		
#-v $(pwd)/../:"/catkin_ws/src/rovio" \

 	export containerId=$(docker ps -l -q)
}

if docker inspect "ros:rovtio" > /dev/null 2>&1
then
	echo Image already exists 
	run_image
else
	echo image doesnt exists, building it first...
	docker build --network=host -t ros:rovtio .
	echo Docker image built. Restart script to run it
fi

