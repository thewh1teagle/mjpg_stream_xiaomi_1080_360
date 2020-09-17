FROM debian:buster


RUN apt-get update && apt-get install -y  \
	bash-completion vim nano git curl wget unzip \
	g++-arm-linux-gnueabihf make file tmux \
	gcc software-properties-common

RUN dpkg --add-architecture armhf
RUN apt-get update
RUN apt-get install -y libev-dev:armhf
RUN mkdir /src

WORKDIR /src

# Usage: 
# sudo docker build . -t mjpg
# sudo docker run -it -v $(pwd):/src mjpg
