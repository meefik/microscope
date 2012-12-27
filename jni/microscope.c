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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <linux/videodev.h>

#define DEFAULT_FILE "/dev/video1"
#define PROGNAME "libmicroscope"
#define VERSION "1.0.4"

int fd = -1;

JNIEXPORT jstring JNICALL Java_ru_lomo_microscope_Microscope_get
  (JNIEnv * env, jobject obj) {

	/*** variable definitions ***/
	struct video_capability cap;
	struct video_window win;
	struct video_picture vpic;

	/* initialize variables */
	cap.name[0] = '\0';
	cap.type = cap.maxwidth = cap.minwidth =
	cap.maxheight = cap.minheight = 0;
	win.width = win.height = 0;
	vpic.brightness = vpic.hue = vpic.colour = vpic.contrast = vpic.whiteness =
	vpic.depth = vpic.palette = 0;

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

	char* brightness[5];
	char* hue[5];
	char* colour[5];
	char* contrast[5];
	char* whiteness[5];
	char* depth[5];
	char* palette[5];
	sprintf(brightness, "%d;%d;%d;%d;%d;%d;%d", vpic.brightness,vpic.hue,vpic.colour,vpic.contrast,
			vpic.whiteness,vpic.depth,vpic.palette);

	return (*env)->NewStringUTF(env, brightness);
}

JNIEXPORT jstring JNICALL Java_ru_lomo_microscope_Microscope_set
  (JNIEnv * env, jobject obj, jint brightness, jint hue, jint colour, jint contrast, jint whiteness,
		  jint depth, jint palette) {

	/*** variable definitions ***/
	int j;	/* iterator */
	int f;
	struct video_capability cap;
	struct video_window win;
	struct video_picture vpic;

	char palettes[17][8] =
	{
		{""}, {"GREY"}, {"HI240"}, {"RGB565"}, {"RGB24"}, {"RGB32"}, {"RGB555"},
		{"YUV422"}, {"YUYV"}, {"UYVY"}, {"YUV420"}, {"YUV411"}, {"RAW"},
		{"YUV422P"}, {"YUV411P"}, {"YUV420P"}, {"YUV410P"}
	};

	/* RGB and image data variables */
	unsigned int i, src_depth;

	/* image format variables */
	int set_image_format = 0;
	//int32_t brightness = -1, hue = -1, colour = -1, contrast = -1,
	//		whiteness = -1, depth = -1, palette = -1;
	uint32_t width=0, height=0;

	/* initialize variables */
	cap.name[0] = '\0';
	cap.type = cap.maxwidth = cap.minwidth =
	cap.maxheight = cap.minheight = 0;
	win.width = win.height = 0;
	vpic.brightness = vpic.hue = vpic.colour = vpic.contrast = vpic.whiteness =
	vpic.depth = vpic.palette = 0;

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

	if (brightness > -1) vpic.brightness = brightness;
	if (hue > -1) vpic.hue = hue;
	if (colour > -1) vpic.colour = colour;
	if (contrast > -1) vpic.contrast = contrast;
	if (whiteness > -1) vpic.whiteness = whiteness;
	if (depth > -1) vpic.depth = depth;
	if (palette > -1) vpic.palette = palette;

	if(ioctl(fd, VIDIOCSPICT, &vpic)==-1) {
		//perror(PROGNAME ": " "VIDIOSPICT");
		return (*env)->NewStringUTF(env, "Error VIDIOSPICT");
	}

	return (*env)->NewStringUTF(env, "Set SUCCESS");
}

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

	/* initialize variables */
	cap.name[0] = '\0';
	cap.type = cap.channels = cap.audios = cap.maxwidth = cap.minwidth =
			cap.maxheight = cap.minheight = 0;

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
