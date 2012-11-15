/*
 LOMO Microscope Expert Image Grabber Library

 Device:
 idVendor=0ac8, idProduct=301b
 Manufacturer: Vimicro Corp.

 Drivers:
 gspca_main, gspca_zc3xx

 (C) 2011 Anton Sk. <anton@cde.ifmo.ru>
 */

#include <jni.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <time.h>

#include <linux/types.h>
#include <linux/videodev.h>

#define PROGNAME "libmicroscope"
#define VERSION "1.1"

int fd = -1;

JNIEXPORT jstring JNICALL Java_ru_lomo_microscope_Microscope_capture(
		JNIEnv * env, jobject obj, jobject globalRef, jlong imgSize) {

	void *buffer = (*env)->GetDirectBufferAddress(env, globalRef);
	if (!buffer) {
		return (*env)->NewStringUTF(env, "Out of memory");
	}

	/* capture image*/
	ssize_t bytesread;
	int firsttime = clock();
	bytesread = read(fd, buffer, imgSize);
	float difftime = (float) (clock() - firsttime) / CLOCKS_PER_SEC;
	float fps = 0;
	if (difftime > 0) {
		fps = (float) 1.0 / difftime;
	}
	char str[15];
	sprintf(str, "FPS: %.0f", fps);
	//sprintf(str, "%d", imgSize);

	if (bytesread < 0) {
		close(fd);
		return (*env)->NewStringUTF(env, "Could not read image data");
	}
	return (*env)->NewStringUTF(env, str);
}

JNIEXPORT jstring JNICALL Java_ru_lomo_microscope_Microscope_open(JNIEnv * env,
		jobject obj, jstring device) {

	/*** variable definitions ***/

	const char *devicename = (*env)->GetStringUTFChars(env, device, 0);
	struct video_capability cap;
	struct video_window win;
	struct video_picture vpic;

	/*** begin of program code ***/
	/* set default devicename */
	//devicename = strdup(devicename);

	/* initialize variables */
	cap.name[0] = '\0';
	cap.type = cap.channels = cap.audios = cap.maxwidth = cap.minwidth =
			cap.maxheight = cap.minheight = 0;
	win.x = win.y = win.width = win.height = win.chromakey = win.flags = 0;
	vpic.brightness = vpic.hue = vpic.colour = vpic.contrast = vpic.whiteness =
			vpic.depth = vpic.palette = 0;

	/* open video device */
	fd = open(devicename, O_RDONLY);
	if (fd < 0) {
		return (*env)->NewStringUTF(env, "Device open error");
	}
	/* not needed any more */
	(*env)->ReleaseStringUTFChars(env, device, devicename);

	/* query device capabilities */
	if (ioctl(fd, VIDIOCGCAP, &cap) < 0) {
		close(fd);
		return (*env)->NewStringUTF(env, "Not a video4linux device");
	}

	/* query current video window settings */
	if (ioctl(fd, VIDIOCGWIN, &win) < 0) {
		close(fd);
		return (*env)->NewStringUTF(env, "Video window settings error");
	}

	/* query image properties */
	if (ioctl(fd, VIDIOCGPICT, &vpic) < 0) {
		close(fd);
		return (*env)->NewStringUTF(env, "Image properties error");
	}

	/* exit if device can not capture to memory */
	if (!(cap.type & VID_TYPE_CAPTURE)) {
		return (*env)->NewStringUTF(env,
				"Device cannot capture");
	}

	/* capture image*/
	ssize_t bytesread;
	read(fd, NULL, 0);

	if (bytesread < 0) {
		close(fd);
		return (*env)->NewStringUTF(env, "Could not read image data");
	}

	return (*env)->NewStringUTF(env, "Success");
}

JNIEXPORT jstring JNICALL Java_ru_lomo_microscope_Microscope_close(JNIEnv * env,
		jobject obj) {

	/* done reading from v4l device */
	close(fd);

	return (*env)->NewStringUTF(env, "SUCCESS");
}

JNIEXPORT jobject JNICALL Java_ru_lomo_microscope_Microscope_allocNativeBuffer(
		JNIEnv* env, jobject thiz, jlong size) {
	void* buffer = malloc(size);
	jobject directBuffer = (*env)->NewDirectByteBuffer(env, buffer, size);
	jobject globalRef = (*env)->NewGlobalRef(env, directBuffer);
	return globalRef;
}

JNIEXPORT void JNICALL Java_ru_lomo_microscope_Microscope_freeNativeBuffer(
		JNIEnv* env, jobject thiz, jobject globalRef) {
	void *buffer = (*env)->GetDirectBufferAddress(env, globalRef);
	(*env)->DeleteGlobalRef(env, globalRef);
	free(buffer);
}
