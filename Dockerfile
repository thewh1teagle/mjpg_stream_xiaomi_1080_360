FROM ubuntu:18.04


RUN apt-get update && apt-get install -y  \
	bash-completion vim nano git curl wget unzip \
	g++-arm-linux-gnueabihf make file tmux \
	gcc software-properties-common

RUN mkdir /src

WORKDIR /src

# Usage: 
# sudo docker build . -t mjpg
# sudo docker run -it mjpg -v $(pwd):/src
