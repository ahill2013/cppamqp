TEST=src/test/proctest.o mq.o processor.o
MC=src/mc/mc.o mq.o processor.o
VISION= vision.o mq.o processor.o
VISTEST=src/test/visual_test.o mq.o processor.o
# DO NOT CHANGE THE ORDER OF THESE -I INCLUDE DIRECTORIES OR EVERYTHING BREAKS IN THE VISION COMPILATION.
VISINC := -I/usr/local/include/eigen3 -I/usr/include/opencv -I/usr/include/eigen3 -I/usr/local/zed/include -I/usr/local/cuda-7.0/include 
VISLD := -L/usr/local/zed/lib -rdynamic /usr/local/zed/lib/libsl_zed.so /usr/local/zed/lib/libsl_depthcore.so /usr/local/zed/lib/libsl_calibration.so /usr/local/zed/lib/libsl_tracking.so /usr/local/zed/lib/libsl_svorw.so /usr/local/zed/lib/libcudpp.so /usr/local/zed/lib/libcudpp_hash.so /usr/lib/libopencv_vstab.so.2.4.13 /usr/lib/libopencv_tegra.so.2.4.13 /usr/lib/libopencv_imuvstab.so.2.4.13 /usr/lib/libopencv_facedetect.so.2.4.13 /usr/lib/libopencv_esm_panorama.so.2.4.13 /usr/lib/libopencv_detection_based_tracker.so.2.4.13 /usr/lib/libopencv_videostab.so.2.4.13 /usr/lib/libopencv_video.so.2.4.13 /usr/lib/libopencv_ts.a /usr/lib/libopencv_superres.so.2.4.13 /usr/lib/libopencv_stitching.so.2.4.13 /usr/lib/libopencv_photo.so.2.4.13 /usr/lib/libopencv_objdetect.so.2.4.13 /usr/lib/libopencv_ml.so.2.4.13 /usr/lib/libopencv_legacy.so.2.4.13 /usr/lib/libopencv_imgproc.so.2.4.13 /usr/lib/libopencv_highgui.so.2.4.13 /usr/lib/libopencv_gpu.so.2.4.13 /usr/lib/libopencv_flann.so.2.4.13 /usr/lib/libopencv_features2d.so.2.4.13 /usr/lib/libopencv_core.so.2.4.13 /usr/lib/libopencv_contrib.so.2.4.13 /usr/lib/libopencv_calib3d.so.2.4.13 /usr/local/cuda-7.0/lib64/libcudart.so /usr/local/cuda-7.0/lib64/libnpps.so /usr/local/cuda-7.0/lib64/libnppi.so /usr/lib/libopencv_tegra.so.2.4.13 /usr/lib/libopencv_stitching.so.2.4.13 /usr/lib/libopencv_gpu.so.2.4.13 /usr/lib/libopencv_photo.so.2.4.13 /usr/lib/libopencv_legacy.so.2.4.13 /usr/local/cuda-7.0/lib64/libcufft.so /usr/lib/libopencv_video.so.2.4.13 /usr/lib/libopencv_objdetect.so.2.4.13 /usr/lib/libopencv_ml.so.2.4.13 /usr/lib/libopencv_calib3d.so.2.4.13 /usr/lib/libopencv_features2d.so.2.4.13 /usr/lib/libopencv_highgui.so.2.4.13 /usr/lib/libopencv_imgproc.so.2.4.13 /usr/lib/libopencv_flann.so.2.4.13 /usr/lib/libopencv_core.so.2.4.13 /usr/local/cuda-7.0/lib64/libcudart.so /usr/local/cuda-7.0/lib64/libnppc.so /usr/local/cuda-7.0/lib64/libnppi.so /usr/local/cuda-7.0/lib64/libnpps.so -ldl -lm -lpthread -lrt -ltbb -Wl,-rpath,/usr/local/zed/lib:/usr/local/cuda-7.0/lib64 
NAVTEST=src/test/nav.o mq.o processor.o
COMTEST=src/test/command_test.o mq.o processor.o
GPS= src/gps/gps.o mq.o processor.o
GOBJS= src/gps/gps2.o mq.o processor.o
JTEST= jsontest.o mq.o
CC=g++
DEBUG=-g -O0
CXXFLAGS=-Wall $(DEBUG) -std=c++11
LDLIBS= -lamqpcpp -lev -lpthread -lSimpleAmqpClient -lrabbitmq

all: gps test vision

test: $(TEST)
	$(CC) $(CXXFLAGS) $(TEST) -o bin/test $(LDLIBS)

vistest: $(VISTEST)
	$(CC) $(CXXFLAGS) $(VISTEST) -o bin/vis_test $(LDLIBS)

navtest: $(NAVTEST)
	$(CC) $(CXXFLAGS) $(NAVTEST) -o bin/nav_test $(LDLIBS)

comtest: $(COMTEST)
	$(CC) $(CXXFLAGS) $(COMTEST) -o bin/com_test $(LDLIBS)

mc: $(MC)
	$(CC) $(CXXFLAGS) $(MC) -o bin/mc $(LDLIBS)

gps: $(GPS)
	$(CC) $(CXXFLAGS) $(GPS) -o bin/gps $(LDLIBS)

vision: $(VISION)
	$(CC) $(CXXFLAGS) $(VISION) -o bin/vision $(VISLD) $(LDLIBS)

gpss: $(GOBJS)
	$(CC) $(CXXFLAGS) $(GOBJS) -o bin/gps2 $(LDLIBS)

jsontest: $(JTEST)
	$(CC) $(CXXFLAGS) $(JTEST) -o bin/jtest $(LDLIBS)

vision.o: src/vision/vision.cpp
	$(CC) $(CXXFLAGS) $(VISINC) -c src/vision/vision.cpp

mq.o: src/mq.cpp include/mq.h
	$(CC) $(CXXFLAGS) -c src/mq.cpp

processor.o: src/processor.cpp include/processor.h
	$(CC) $(CXXFLAGS) -c src/processor.cpp

clean:
	rm -f src/*.o src/mc/*.o src/gps/*.o src/test/*.o *.o bin/*
