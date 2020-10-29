# mjpg_stream_xiaomi_1080_360


This is a mjpeg stream server for xiaomi 1080 360 camera model MJSXJ02CM


## How to build

install docker
```
sudo apt-get install -y docker.io
```
build container. this container will be our environment for compile the program.
```
sudo docker build . -t mjpg
```

Run the container and compile the program
```
sudo docker run -it -v $(pwd):/src mjpg
make -f Makefile_02
```

Create folder with the binaris to be running on the camera
 
```
mkdir mjpeg_stream
cp -rf input_snapshot.so output_http.so mjpg_streamer www mjpeg_stream
```

How to run
```
./mjpg_streamer -i "./input_snapshot.so -d 1000" -o "./output_http.so -w ./www"
```


