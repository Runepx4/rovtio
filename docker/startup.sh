#!/bin/bash
run_image(){
	echo 'running the rovtio node in the  docker image..'
	docker run -it \
		--rm --gpus all\
		--net=host \
		--name rovtio \
		--env="DISPLAY" \
		--env="QT_X11_NO_MITSHM=1" \
		ros:rovtio \
		/bin/bash -c "source /catkin_ws/devel/setup.bash && roslaunch rovtio rovtio-kinect.launch"
}

if docker inspect "ros:rovtio" > /dev/null 2>&1
then
	echo Image already exists 
	run_image
else
	echo image doesnt exists, building it first...
	docker build --network=host --tag ros:rovtio -f ./Dockerfile ..
	echo 'Docker image built. Restart script to run it!'
fi

