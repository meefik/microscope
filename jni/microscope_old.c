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

#include <linux/types.h>
#include <linux/videodev.h>

#define DEFAULT_FILE "/dev/video1"
#define PROGNAME "libmicroscope"
#define VERSION "1.0"
#define MAX_BRIGHTNESS_LOOPS 100
#define MAX_IMAGE_SIZE_NAME_LENGTH 5



typedef struct config_s {
 char *outfile;        /* filename for output */
 char *device;         /* v4l device */
 int brightness;       /* set brightness */
 int hue;              /* set hue */
 int colour;           /* set colour */
 int contrast;         /* set contrast */
 int whiteness;        /* set whiteness */
 int depth;            /* set capture depth */
 int channel;          /* set source channel */
 char *size;           /* set image size */
 int max_size;         /* set maximum supported image size */
 int min_size;         /* set minimum supported image size */
 int auto_bright;      /* use automatic brightness adjust */
 int maxt_ries;        /* max no of tries for brightness adj.*/
 int wait_after_open;  /* give device time to auto-adjust */
 int discard_frames;   /* give device chance to auto-adjust */
} config_t;

typedef struct image_size_s {
  char name[MAX_IMAGE_SIZE_NAME_LENGTH+1];
  uint32_t width;
  uint32_t height;
} image_size_t;

image_size_t image_sizes[] = {
  {"SQCIF", 128, 96},
  {"QSIF", 160, 120},
  {"QCIF", 176, 144},
  {"SIF", 320, 240},
  {"CIF", 352, 288},
  {"QPAL", 384, 288},
  {"VGA", 640, 480},
  {"4CIF", 704, 576},
  {"PAL", 768, 576},
  {"WVGA", 800, 480},
  {"SVGA", 800, 600},
  {"XGA", 1024, 768},
  {"720p", 1280, 720},
  {"SXGA", 1280, 1024},
  {"16CIF", 1408, 1152},
  {"UXGA", 1600, 1200},
  {"1080p", 1920, 1080},
  {"", 0, 0}
};

static int get_brightness_adj(unsigned char *image, long size, int *brightness, int d)
{
  long i, tot = 0;
  for (i=0;i<size*d;i++)
    tot += image[i];
  *brightness = (128 - tot/(size*3))/3;
  return !((tot/(size*3)) >= 126 && (tot/(size*3)) <= 130);
}

/* parse string describing image size */
static int parse_size(const char *s, uint32_t *width, uint32_t *height)
{
  int i;
  char *tmp_s;
  char *tmp_cp;

  /* check if a known size is used */
  for(i=0; image_sizes[i].name[0]; i++) {
    if(strcasecmp(s, image_sizes[i].name) == 0) {
      *width = image_sizes[i].width;
      *height = image_sizes[i].height;
      return 1;
    }
  }

  /* is size given in WIDHTxHEIGHT? */
  tmp_s = strdup(s);
  if((tmp_cp = index(tmp_s, 'x'))) {
    *tmp_cp = '\0';
    tmp_cp++;
    *width = atoi(tmp_s);
    *height = atoi(tmp_cp);
    free(tmp_s);
    return 1;
  } else {
    return 0;
  }
}

/* print queried capabilities */
static void print_capabilities(struct video_capability *cap,
                               struct video_channel **vchan, char *devicename)
{
  int i;	/* iterator */

  /* name */
  fprintf(stderr, " capabilities of \"%s\" (%s):\n", cap->name, devicename);
  /* type of device */
  if(cap->type & VID_TYPE_CAPTURE)
    fprintf(stderr, "  VID_TYPE_CAPTURE\n");
  if(cap->type & VID_TYPE_TUNER)
    fprintf(stderr, "  VID_TYPE_TUNER\n");
  if(cap->type & VID_TYPE_TELETEXT)
    fprintf(stderr, "  VID_TYPE_TELETEXT\n");
  if(cap->type & VID_TYPE_OVERLAY)
    fprintf(stderr, "  VID_TYPE_OVERLAY\n");
  if(cap->type & VID_TYPE_CHROMAKEY)
    fprintf(stderr, "  VID_TYPE_CHROMAKEY\n");
  if(cap->type & VID_TYPE_CLIPPING)
    fprintf(stderr, "  VID_TYPE_CLIPPING\n");
  if(cap->type & VID_TYPE_FRAMERAM)
    fprintf(stderr, "  VID_TYPE_FRAMERAM\n");
  if(cap->type & VID_TYPE_SCALES)
    fprintf(stderr, "  VID_TYPE_SCALES\n");
  if(cap->type & VID_TYPE_MONOCHROME)
    fprintf(stderr, "  VID_TYPE_MONOCHROME\n");
  if(cap->type & VID_TYPE_SUBCAPTURE)
    fprintf(stderr, "  VID_TYPE_SUBCAPTURE\n");
  if(cap->type & VID_TYPE_MPEG_DECODER)
    fprintf(stderr, "  VID_TYPE_MPEG_DECODER\n");
  if(cap->type & VID_TYPE_MPEG_ENCODER)
    fprintf(stderr, "  VID_TYPE_MPEG_ENCODER\n");
  if(cap->type & VID_TYPE_MJPEG_DECODER)
    fprintf(stderr, "  VID_TYPE_MJPEG_DECODER\n");
  if(cap->type & VID_TYPE_MJPEG_ENCODER)
    fprintf(stderr, "  VID_TYPE_MJPEG_ENCODER\n");
  /* number of channels */
  fprintf(stderr, " number of channels: %d\n", cap->channels);
  /* channel names */
  for(i=0; i<cap->channels; i++) {
    fprintf(stderr, "  channel %d: %s\n", i, vchan[i]->name);
  }
  /* number of audio devices */
  fprintf(stderr, " number of audio devices: %d\n", cap->audios);
  /* minimum and maximum image size */
  fprintf(stderr, " image size: ");
  fprintf(stderr, "%d <= width <= %d, %d <= height <= %d\n", cap->minwidth,
                  cap->maxwidth, cap->minheight, cap->maxheight);
}

static void print_window_settings(struct video_window *win)
{
  fprintf(stderr, " coordinates: x=%u, y=%u\n", win->x, win->y);
  fprintf(stderr, " size: %ux%u\n", win->width, win->height);
  fprintf(stderr, " chromakey: %u\n", win->chromakey);
}

static void print_image_properties(struct video_picture *vpic)
{
  fprintf(stderr, " brightness: %u\n", vpic->brightness);
  fprintf(stderr, " hue:        %u\n", vpic->hue);
  fprintf(stderr, " colour:     %u\n", vpic->colour);
  fprintf(stderr, " contrast:   %u\n", vpic->contrast);
  fprintf(stderr, " whiteness:  %u (only for b/w capturing)\n",vpic->whiteness);
  fprintf(stderr, " depth:      %u\n", vpic->depth);
  fprintf(stderr, " palette:    ");
  switch(vpic->palette) {
    case VIDEO_PALETTE_GREY: fprintf(stderr, "VIDEO_PALETTE_GREY"); break;
    case VIDEO_PALETTE_HI240: fprintf(stderr, "VIDEO_PALETTE_HI240"); break;
    case VIDEO_PALETTE_RGB565: fprintf(stderr, "VIDEO_PALETTE_RGB565"); break;
    case VIDEO_PALETTE_RGB24: fprintf(stderr, "VIDEO_PALETTE_RGB24"); break;
    case VIDEO_PALETTE_RGB32: fprintf(stderr, "VIDEO_PALETTE_RGB32"); break;
    case VIDEO_PALETTE_RGB555: fprintf(stderr, "VIDEO_PALETTE_RGB555"); break;
    case VIDEO_PALETTE_YUV422: fprintf(stderr, "VIDEO_PALETTE_YUV422"); break;
    case VIDEO_PALETTE_YUYV: fprintf(stderr, "VIDEO_PALETTE_YUYV"); break;
    case VIDEO_PALETTE_UYVY: fprintf(stderr, "VIDEO_PALETTE_UYVY"); break;
    case VIDEO_PALETTE_YUV420: fprintf(stderr, "VIDEO_PALETTE_YUV420"); break;
    case VIDEO_PALETTE_YUV411: fprintf(stderr, "VIDEO_PALETTE_YUV411"); break;
    case VIDEO_PALETTE_RAW: fprintf(stderr, "VIDEO_PALETTE_RAW"); break;
    case VIDEO_PALETTE_YUV422P: fprintf(stderr,"VIDEO_PALETTE_YUV422P"); break;
    case VIDEO_PALETTE_YUV411P: fprintf(stderr,"VIDEO_PALETTE_YUV411P"); break;
    case VIDEO_PALETTE_YUV420P: fprintf(stderr,"VIDEO_PALETTE_YUV420P"); break;
    case VIDEO_PALETTE_YUV410P: fprintf(stderr,"VIDEO_PALETTE_YUV410P"); break;
    break;
    default: fprintf(stderr, "unknown palette"); break;
  }
  fprintf(stderr, " (%u)\n", vpic->palette);
}

static void print_image_size_help(void)
{
  int i;
  fprintf(stderr, "size KEYWORD is one of (case is ignored):\n");
  for(i=0; image_sizes[i].name[0]; i++) {
    fprintf(stderr, " %-*s for %4dx%-4d\n",
            MAX_IMAGE_SIZE_NAME_LENGTH, image_sizes[i].name,
	    image_sizes[i].width, image_sizes[i].height);
  }
  fprintf(stderr, " min   for minimum size supported by camera\n"
                  " max   for maximum size supported by camera\n");
}

JNIEXPORT jstring JNICALL Java_ru_lomo_microscope_Microscope_capture
  (JNIEnv * env, jobject obj, jobject globalRef) {

	/*** variable definitions ***/
	int j;	/* iterator */
	int fd = -1;
	FILE *ofd = NULL;
	int f;
	char *devicename;
	char *outfilename;
	struct video_capability cap;
	struct video_window win;
	struct video_picture vpic;
	struct video_channel **vchan = NULL;

	char palettes[17][8] =
	{
		{""}, {"GREY"}, {"HI240"}, {"RGB565"}, {"RGB24"}, {"RGB32"}, {"RGB555"},
		{"YUV422"}, {"YUYV"}, {"UYVY"}, {"YUV420"}, {"YUV411"}, {"RAW"},
		{"YUV422P"}, {"YUV411P"}, {"YUV420P"}, {"YUV410P"}
	};

	/* RGB and image data variables */
	//unsigned char *buffer;
	unsigned int i, src_depth;

	/* image format variables */
	int set_image_format = 0;
	int32_t brightness = -1, hue = -1, colour = -1, contrast = -1,
			whiteness = -1, depth = -1, palette = -1;
	uint32_t width=0, height=0;
	int channel = -1;

	/* program operation flags */
	int show_info = 0;
	int set_only = 0;
	int verbose = 0;
	int quiet = 0;
	int auto_bright = 0;
	unsigned int max_tries = MAX_BRIGHTNESS_LOOPS;
	int set_max_size = 0;
	int set_min_size = 0;
	unsigned int open_delay = 0;
	int discard_number = 0;

	/*** begin of program code ***/
	/* set default devicename */
	devicename = strdup(DEFAULT_FILE);

	/* initialize variables */
	cap.name[0] = '\0';
	cap.type = cap.channels = cap.audios = cap.maxwidth = cap.minwidth =
	cap.maxheight = cap.minheight = 0;
	win.x = win.y = win.width = win.height = win.chromakey = win.flags = 0;
	vpic.brightness = vpic.hue = vpic.colour = vpic.contrast = vpic.whiteness =
	vpic.depth = vpic.palette = 0;

	/* open video device */
	if(verbose)
		fprintf(stderr, PROGNAME ": " "opening video device...\n");
	fd = open(devicename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, PROGNAME ": ");
		perror(devicename);
		//exit(EXIT_FAILURE);
		return (*env)->NewStringUTF(env, "FAILURE 1");
	}

	/* give video device some time to auto-adjust exposure etc. */
	if(open_delay) {
		if(verbose)
			fprintf(stderr, PROGNAME ": " "waiting %u seconds...\n", open_delay);
		sleep(open_delay);
	}

	/* query device capabilities */
	if(verbose)
		fprintf(stderr, PROGNAME ": " "querying device capabilities...\n");
	if (ioctl(fd, VIDIOCGCAP, &cap) < 0) {
		perror(PROGNAME ": " "VIDIOGCAP");
		if(!quiet)
			fprintf(stderr, PROGNAME ": " "(%s not a video4linux device?)\n",
					devicename);
		close(fd);
		//exit(EXIT_FAILURE);
		return (*env)->NewStringUTF(env, "FAILURE 2");
	}

	/* query channel information */
	if(show_info || (channel>-1)) {
		if(verbose)
			fprintf(stderr, PROGNAME ": " "querying channel information...\n");
		if(!(vchan = calloc(cap.channels, sizeof(struct video_channel *)))) {
			perror(PROGNAME ": " "calloc()");
			close(fd);
			//exit(EXIT_FAILURE);
			return (*env)->NewStringUTF(env, "FAILURE 3");
    	}
		for(j=0; j<cap.channels; j++) {
			if(verbose)
				fprintf(stderr, PROGNAME ": " "querying channel %d...\n", j);
			if(!(vchan[j] = calloc(1, sizeof(struct video_channel)))) {
				perror(PROGNAME ": " "calloc()");
				close(fd);
				//exit(EXIT_FAILURE);
				return (*env)->NewStringUTF(env, "FAILURE 4");
			}
			vchan[j]->channel = j;
			if(ioctl(fd, VIDIOCGCHAN, vchan[j]) < 0) {
				if(!quiet)
					fprintf(stderr, PROGNAME ": warning: could not query channel %d\n",j);
			}
		}
	}

	if(show_info)
		print_capabilities(&cap, vchan, devicename);

	/* query current window settings */
	if(verbose)
		fprintf(stderr, PROGNAME ": querying current video window settings...\n");
	if (ioctl(fd, VIDIOCGWIN, &win) < 0) {
		perror(PROGNAME ": " "VIDIOCGWIN");
		close(fd);
		//exit(EXIT_FAILURE);
		return (*env)->NewStringUTF(env, "FAILURE 5");
	}
	if(show_info)
		print_window_settings(&win);

	/* query image properties */
	if(verbose)
		fprintf(stderr, PROGNAME ": " "querying current image properties...\n");
	if(ioctl(fd, VIDIOCGPICT, &vpic) < 0) {
		perror(PROGNAME ": " "VIDIOCGPICT");
		close(fd);
		//exit(EXIT_FAILURE);
		return (*env)->NewStringUTF(env, "FAILURE 6");
	}
	if(show_info) {
		print_image_properties(&vpic);
		//exit(EXIT_SUCCESS);
		return (*env)->NewStringUTF(env, "FAILURE 7");
	}

	/* exit if device can not capture to memory */
	if(!(cap.type & VID_TYPE_CAPTURE)) {
		if(!quiet) {
			fprintf(stderr, PROGNAME ": error: video4linux device %s cannot capture"
							" to memory\n", devicename);
		}
		//exit(EXIT_FAILURE);
		return (*env)->NewStringUTF(env, "FAILURE 8");
	}
	free(devicename);	/* not needed any more */

	/* set source channel */
	if(channel > -1) {
		if(channel < cap.channels) {
			if(verbose)
				fprintf(stderr, PROGNAME ": " "selecting source channel %d (%s)...\n",
								channel, vchan[channel]->name);
			if(ioctl(fd, VIDIOCSCHAN, vchan[channel]) < 0) {
				perror(PROGNAME ": " "VIDIOCSCHAN");
				close(fd);
				//exit(EXIT_FAILURE);
				return (*env)->NewStringUTF(env, "FAILURE 9");
			}
			if(verbose)
				fprintf(stderr, PROGNAME ": " " selected channel %d (%s)\n", channel,
								vchan[channel]->name);
		} else {
			if(!quiet)
				fprintf(stderr, PROGNAME ": "
								"there is no channel %d (found %d channel%s)\n",
								channel, cap.channels, (cap.channels == 1) ? "" : "s");
		}
	}

	/* free memory used for channel descriptions */
	if(vchan) {
		for(j=0; j<cap.channels; j++) {
			free(vchan[j]);
		}
	}

	/* set image format */
	if(set_image_format) {
    	if(set_max_size) {
			if(verbose)
				fprintf(stderr, PROGNAME ": "
								"using maximum supported image size %ux%u...\n",
								cap.maxwidth, cap.maxheight);
			width = cap.maxwidth;
			height = cap.maxheight;
		}
		if(set_min_size) {
			if(verbose)
				fprintf(stderr, PROGNAME ": "
								"using minimum supported image size %ux%u...\n",
								cap.minwidth, cap.minheight);
			width = cap.minwidth;
			height = cap.minheight;
		}
		if(width > 0) {
			if((width >= (uint32_t) cap.minwidth) &&
					(width <= (uint32_t) cap.maxwidth)) {
				if(verbose)
					fprintf(stderr, PROGNAME ": " "setting image width %u...\n", width);
				win.width = width;
			} else if(!quiet) {
				fprintf(stderr, PROGNAME ": "
								"ignoring image width %u not in [%u,%u]\n", width,
	        					cap.minwidth, cap.maxwidth);
			}
		}
		if(height > 0) {
			if((height >= (uint32_t) cap.minheight) &&
					(height <= (uint32_t) cap.maxheight)) {
				if(verbose)
					fprintf(stderr, PROGNAME ": " "setting image height %u...\n", height);
									win.height = height;
			} else if(!quiet) {
				fprintf(stderr, PROGNAME ": "
								"ignoring image height %u not in [%u,%u]\n", height,
								cap.minheight, cap.maxheight);
			}
		}
		if(brightness > -1) {
			if(brightness <= 65535) {
				if(verbose)
					fprintf(stderr, PROGNAME ": setting brightness %u...\n", brightness);
									vpic.brightness = brightness;
			} else if(!quiet) {
				fprintf(stderr, PROGNAME ": ignoring brightness %u > 65535\n",
								brightness);
			}
		}
		if(hue > -1) {
			if(hue <= 65535) {
				if(verbose)
					fprintf(stderr, PROGNAME ": " "setting hue %u...\n", hue);
									vpic.hue = hue;
			} else if(!quiet) {
				fprintf(stderr, PROGNAME ": " "ignoring hue %u > 65535\n", hue);
			}
		}
		if(colour > -1) {
			if(colour <= 65535) {
				if(verbose)
					fprintf(stderr, PROGNAME ": " "setting colour %u...\n", colour);
									vpic.colour = colour;
			} else if(!quiet) {
				fprintf(stderr, PROGNAME ": " "ignoring colour %u > 65535\n", colour);
			}
		}
		if(contrast > -1) {
			if(contrast <= 65535) {
				if(verbose)
					fprintf(stderr, PROGNAME ": " "setting contrast %u...\n", contrast);
									vpic.contrast = contrast;
			} else if(!quiet) {
					fprintf(stderr, PROGNAME ": ignoring contrast %u > 65535\n", contrast);
			}
		}
		if(whiteness > -1) {
			if(whiteness <= 65535) {
				if(verbose)
					fprintf(stderr, PROGNAME ": " "setting whiteness %u...\n", whiteness);
									vpic.whiteness = whiteness;
			} else if(!quiet) {
				fprintf(stderr, PROGNAME ": " "ignoring whiteness %u > 65535\n",
								whiteness);
			}
		}
		if(depth > -1) {
			if(depth <= 65535) {
				if(verbose)
					fprintf(stderr, PROGNAME ": " "setting capture depth %u...\n", depth);
									vpic.depth = depth;
			} else if(!quiet) {
				fprintf(stderr, PROGNAME ": ignoring capture depth %u > 65535\n",depth);
			}
		}

		/* set image size */
		if((width > 0) || (height > 0)) {
			if(verbose)
				fprintf(stderr, PROGNAME ": " "setting image size...\n");
			if(ioctl(fd, VIDIOCSWIN, &win) < 0) {
				perror(PROGNAME ": " "VIDIOCSWIN");
				if(!quiet)
					fprintf(stderr, PROGNAME ": " "warning: could not set image size\n");
			}

			/* check result */
			if(ioctl(fd, VIDIOCGWIN, &win) < 0) {
				perror(PROGNAME ": " "VIDIOCGWIN");
				if(!quiet)
					fprintf(stderr, PROGNAME ": " "error: could not read window settings,"
									" exiting\n");
				//exit(EXIT_FAILURE);
				return (*env)->NewStringUTF(env, "FAILURE 10");
      		} else {
				if(win.width != width) {
	  				if(!quiet)
	    				fprintf(stderr, PROGNAME ": " "warning: set image width to %u\n",
		    							win.width);
				} else if(verbose) {
					fprintf(stderr, PROGNAME ": " "successfully set image width %u\n",
									win.width);
				}
				if(win.height != height) {
					if(!quiet)
						fprintf(stderr, PROGNAME ": " "warning: set image height to %u\n",
										win.height);
				} else if(verbose) {
					fprintf(stderr, PROGNAME ": " "successfully set image height %u\n",
									win.height);
				}
			}
		}

		/* set image properties */
		if(verbose)
			fprintf(stderr, PROGNAME ": " "setting image properties...\n");
		if(ioctl(fd, VIDIOCSPICT, &vpic) < 0) {
			perror(PROGNAME ": " "VIDIOCSPICT");
			if(!quiet)
				fprintf(stderr, PROGNAME ": warning: could not set image properties\n");
		}

		/* check if request could be fulfilled */
		if(ioctl(fd, VIDIOCGPICT, &vpic) < 0) {
			perror(PROGNAME ": " "VIDIOCGPICT");
			if(!quiet)
				fprintf(stderr, PROGNAME ": "
								"error: could not read image properties, exiting\n");
			close(fd);
			//exit(EXIT_FAILURE);
			return (*env)->NewStringUTF(env, "FAILURE 11");
		} else {
			if(brightness > -1) {
				if(brightness != vpic.brightness) {
					if(!quiet)
						fprintf(stderr, PROGNAME ": " "warning: set brightness to %u\n",
										vpic.brightness);
				} else if(verbose) {
					fprintf(stderr, PROGNAME ": " "brightness successfully set to %u\n",
									vpic.brightness);
				}
			}
			if(hue > -1) {
				if(hue != vpic.hue) {
					if(!quiet)
						fprintf(stderr, PROGNAME ": " "warning: set hue to %u\n",vpic.hue);
				} else if(verbose) {
					fprintf(stderr, PROGNAME ": " "hue successfully set to %u\n",
									vpic.hue);
				}
			}
			if(colour > -1) {
				if(colour != vpic.colour) {
					if(!quiet)
						fprintf(stderr, PROGNAME ": "
										"warning: set colour to %u\n", vpic.colour);
				} else if(verbose) {
					fprintf(stderr, PROGNAME ": " "colour successfully set to %u\n",
									vpic.colour);
				}
			}
			if(contrast > -1) {
				if(contrast != vpic.contrast) {
					if(!quiet)
						fprintf(stderr, PROGNAME ": " "warning: set contrast to %u\n",
										vpic.contrast);
				} else if(verbose) {
					fprintf(stderr, PROGNAME ": " "contrast successfully set to %u\n",
									vpic.contrast);
				}
			}
			if(whiteness > -1) {
				if(whiteness != vpic.whiteness) {
					if(!quiet)
						fprintf(stderr, PROGNAME ": " "warning: set whiteness to %u\n",
										vpic.whiteness);
				} else if(verbose) {
					fprintf(stderr, PROGNAME ": " "whiteness successfully set to %u\n",
									vpic.whiteness);
				}
			}
			if(depth > -1) {
				if(depth != vpic.depth) {
					if(!quiet)
						fprintf(stderr, PROGNAME ": " "warning: set depth to %u\n",
										vpic.depth);
				} else if(verbose) {
					fprintf(stderr, PROGNAME ": " "depth successfully set to %u\n",
									vpic.depth);
				}
			}
			if(palette > -1) {
				if(palette != vpic.palette) {
					if(!quiet)
						fprintf(stderr, PROGNAME ": " "warning: set palette to %s\n",
										palettes[vpic.palette]);
				} else if(verbose) {
					fprintf(stderr, PROGNAME ": " "palette successfully set to %s\n",
									palettes[vpic.palette]);
				}
			}
		}
	}
	if(set_only)
		//exit(EXIT_SUCCESS);
		return (*env)->NewStringUTF(env, "FAILURE 12");

	src_depth = vpic.depth;
	if(verbose) {
		fprintf(stderr, PROGNAME ": " "using palette %s (depth %d bpp)\n",
						palettes[vpic.palette], vpic.depth);
	}

	if(verbose)
		fprintf(stderr, PROGNAME ": " "allocating buffer for image...\n");
	//buffer = malloc((win.width * win.height * src_depth)/8);
	void *buffer = (*env)->GetDirectBufferAddress(env, globalRef);
	if (!buffer) {
		fprintf(stderr, PROGNAME ": " "out of memory.\n");
		//exit(EXIT_FAILURE);
		return (*env)->NewStringUTF(env, "FAILURE 13");
	}

	/* capture image; adjust brightness when told so and using some RGB palette */
	i = 0;
	do {
		int newbright;
		ssize_t bytesread;
		int frame_number;

		/* discard some frames to let v4l device auto-adjust exposure etc. */
		for(frame_number=0 ; frame_number <= discard_number; frame_number++) {

			/* capture an image frame */
			if(verbose)
				fprintf(stderr, PROGNAME ": " "capturing image...\n");
				bytesread = read(fd, buffer, (win.width * win.height * src_depth)/8);
			if(verbose)
				fprintf(stderr, PROGNAME ": " "read %d bytes\n", bytesread);
			if(bytesread < 0) {
				perror(PROGNAME ": " "could not read image data");
				close(fd);
				//exit(EXIT_FAILURE);
				return (*env)->NewStringUTF(env, "FAILURE 14");
			}

			if(verbose && (frame_number < discard_number))
				fprintf(stderr, PROGNAME ": " "discarding frame %d/%d\n",
								frame_number+1, discard_number);
		}

		/* software brightness auto-adjust if requested and some RGB palette used */
		if((auto_bright) && (brightness < 0) &&
				(vpic.palette!=VIDEO_PALETTE_UYVY)&&(vpic.palette!=VIDEO_PALETTE_YUYV)&&
				(vpic.palette!=VIDEO_PALETTE_YUV422)) {
			if(verbose)
				fprintf(stderr, PROGNAME ": " "adjusting brightness (try %u/%u)...\n",
								i+1,max_tries);
				f = get_brightness_adj(buffer, win.width * win.height, &newbright,
										src_depth/8);
			if (f) {
				vpic.brightness += (newbright << 8);
				if(ioctl(fd, VIDIOCSPICT, &vpic)==-1) {
					perror(PROGNAME ": " "VIDIOSPICT");
					break;
				}
			}
		} else {
			break; /* no software brightness auto-adjust */
		}
		i++;
	} while (f && (i < max_tries));

	/* done reading from v4l device */
	close(fd);
  
	return (*env)->NewStringUTF(env, "SUCCESS");
}

JNIEXPORT jstring JNICALL Java_ru_lomo_microscope_Microscope_open
  (JNIEnv * env, jobject obj) {

	int fd = -1;
	char *devicename = strdup(DEFAULT_FILE);
	
	fd = open(devicename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, PROGNAME ": ");
		perror(devicename);
		return (*env)->NewStringUTF(env, "FAILURE");
	}
	return (*env)->NewStringUTF(env, "SUCCESS");
}

JNIEXPORT jstring JNICALL Java_ru_lomo_microscope_Microscope_close
  (JNIEnv * env, jobject obj) {

	
	return (*env)->NewStringUTF(env, "SUCCESS");
}

JNIEXPORT jobject JNICALL Java_ru_lomo_microscope_Microscope_allocNativeBuffer(JNIEnv* env, jobject thiz, jlong size)
{
	void* buffer = malloc(size);
	jobject directBuffer = (*env)->NewDirectByteBuffer(env, buffer, size);
	jobject globalRef = (*env)->NewGlobalRef(env, directBuffer);
	return globalRef;
}

JNIEXPORT void JNICALL Java_ru_lomo_microscope_Microscope_freeNativeBuffer(JNIEnv* env, jobject thiz, jobject globalRef)
{
	void *buffer = (*env)->GetDirectBufferAddress(env, globalRef);
	(*env)->DeleteGlobalRef(env, globalRef);
	free(buffer);
}
