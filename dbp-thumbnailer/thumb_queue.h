#ifndef __THUMB_QUEUE_H__
#define	__THUMB_QUEUE_H__

enum ThumbError {
	THUMB_ERROR_BAD_MIME,
	THUMB_ERROR_I_AM_A_TEAPOT,
	THUMB_ERROR_BAD_DATA,
	THUMB_ERROR_OMNOMNOM_LOOP,
	THUMB_ERROR_SAVE_ERROR,
	THUMB_ERROR_BAD_TASTE,
};

int thumb_queue(const char *uri, const char *flavour);
void thumb_queue_init();

#endif
