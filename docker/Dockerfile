#ARG BASE_IMAGE=dustynv/ros:melodic-ros-base-l4t-r32.4.4
ARG BASE_IMAGE=ros:melodic-perception-bionic
FROM ${BASE_IMAGE}

SHELL ["/bin/bash", "-c"]


RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    git python-catkin-tools build-essential \
    freeglut3-dev libglew-dev

RUN echo "source /opt/ros/melodic/setup.bash" >> ~/.bashrc

RUN mkdir -p /catkin_ws/src/ && cd /catkin_ws/src/ && \
    git clone https://github.com/Runepx4/rovtio.git -b circle_mask && \
    git clone https://github.com/ethz-asl/kindr && \
    cd /catkin_ws/src/rovtio/ && git submodule update --init --recursive && ls && \
    cd /catkin_ws/ && catkin config --extend /opt/ros/melodic

RUN cd /catkin_ws/ && . /opt/ros/melodic/setup.sh && \
    rosdep install --from-paths src --ignore-src -r -y && \
    catkin build rovtio --cmake-args -DCMAKE_BUILD_TYPE=Release -DMAKE_SCENE=ON -DROVIO_NCAM=1 -DROVIO_NMAXFEATURE=25
    
COPY ./cfg/rovtio /catkin_ws/src/rovtio/cfg/rovtio
COPY ./launch /catkin_ws/src/rovtio/launch

WORKDIR /catkin_ws
