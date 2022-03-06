/*
 * audio_callback.c
 *
 *  Created on: Mar 23, 2016
 *      Author: a552095
 */
#include "audio_callback.h"
#include <dlog.h>

#include <stdio.h>
#include <math.h>

#include "fourier.h"
#define BUFFSIZE 32768

char **noteName, **octName;

/*
 * Berechnet die Anzahl Halbtöne, die der gesuchte Ton vom Kammerton entfernt ist und zählt vier Oktaven dazu.
 */
static double calculateHalfTones(float freq) {
	return 12. / M_LN2 * (log(freq) - log(440.)) + 48.5;
}

const static char *accidental[] = {
		"", "#", "", "", "#", "", "#", "", "", "#", "", "#"
};

char language[3];
int waitCycles = 0;
/*
 * @brief Rotate hands of the watch
 * @param[in] hand The hand you want to rotate
 * @param[in] degree The degree you want to rotate
 * @param[in] cx The rotation's center horizontal position
 * @param[in] cy The rotation's center vertical position
 */
void view_rotate_hand(Evas_Object *hand, double degree, Evas_Coord cx, Evas_Coord cy)
{
	Evas_Map *m = NULL;

	m = evas_map_new(4);
	evas_map_util_points_populate_from_object(m, hand);
	evas_map_util_rotate(m, degree, cx, cy);
	evas_object_map_set(hand, m);
	evas_object_map_enable_set(hand, EINA_TRUE);
	evas_map_free(m);
}

Eina_Bool displayNote(void *data) {
	appdata_s *ad = data;
//	dlog_print(DLOG_DEBUG, LOG_TAG, "Timer was triggered: NewFreq: %f, oldFreq: %f", ad->newFreq, ad->dispFreq);
	if (ad->newFreq == ad->dispFreq) {
		if (++waitCycles % 20 == 0) {
			evas_object_show(ad->waiting);
			audio_in_unprepare(ad->input);
			audio_in_destroy(ad->input);
			audio_in_create(SAMPLE_RATE, AUDIO_CHANNEL_MONO, AUDIO_SAMPLE_TYPE_S16_LE, &ad->input);
			audio_in_set_stream_cb(ad->input, io_stream_callback, ad);
			audio_in_prepare(ad->input);
		}
		return ad->isActive;
	}
	float freq = ad->newFreq;
	char hertzstr[32];
	double deg = 0.;
	if (freq > 10.) {
		double halftones = calculateHalfTones(freq);
		int fht = (int)halftones;
		int octaveidx = (fht - 3) / 12;
		int noteidx = fht % 12;
		sprintf(hertzstr, "%.1f Hz", freq);
		evas_object_text_text_set(ad->freq, hertzstr);
		evas_object_text_text_set(ad->note, noteName[noteidx]);
		evas_object_text_text_set(ad->accidental, accidental[noteidx]);
		if (octaveidx < MAXOCTAVE)
			evas_object_text_text_set(ad->octave, octName[octaveidx]);
		else
			evas_object_text_text_set(ad->octave, "");
		deg = (halftones - 0.5 - fht) * 80.;
	} else {
		evas_object_text_text_set(ad->freq, "-------");
		evas_object_text_text_set(ad->note, "");
		evas_object_text_text_set(ad->accidental, "");
		evas_object_text_text_set(ad->octave, "");
	}
	Evas_Map *rot = evas_map_new(4);
	evas_map_util_points_populate_from_object(rot, ad->hand);
	evas_map_point_image_uv_set(rot, 0, 0., 0.);
	evas_map_point_image_uv_set(rot, 1, 39., 0.);
	evas_map_point_image_uv_set(rot, 2, 39., 88.);
	evas_map_point_image_uv_set(rot, 3, 0., 88.);
	evas_map_util_rotate(rot, deg, ad->centerX, ad->centerY);
	evas_object_map_set(ad->hand, rot);
	evas_object_map_enable_set(ad->hand, EINA_TRUE);
	ad->dispFreq = freq;
	return ad->isActive;
}

float data[2][BUFFSIZE];
int nproc = 0, activebuf = 0, idx = 0;

void reset_data() {
	activebuf = idx = 0;
}


float absquad(float *dptr) {
	return dptr[0] * dptr[0] + dptr[1] * dptr[1];
}

void evaluate_audio(float *data, appdata_s *ad) {
	realft(data - 1, BUFFSIZE, 1);
	// GetMax
	float maxval = 0;
	int maxidx = 0;
	for (int i = 40; i < BUFFSIZE; i += 2) {
		float val = absquad(data + i) / (i + 100);
		if (val > maxval) {
			maxval = val;
			maxidx = i / 2;
		}
	}
	if (maxval * maxidx > 1.e10) {
		// Aufgrund der Werte der Nachbarpunkte und einer quadratischen Interpolation
		// versuchen wir die Frequenz noch genauer abzuschätzen.
		float ym = sqrt(absquad(data + maxidx - 1));
		float y0 = sqrt(maxval);
		float yp = sqrt(absquad(data + maxidx + 1));
		float corr = (ym - yp) / (2. * ym - 4. * y0 + 2. * yp);
		float freq = ((float) maxidx + corr) * SAMPLE_RATE / BUFFSIZE;
		ad->newFreq = freq;
	} else {
		ad->newFreq = 0.f;
	}
}

void io_stream_callback(audio_in_h handle, size_t nbytes, void *userdata) {
	const short *buffer;
	float *evalbuf = NULL;
	appdata_s *ad = (appdata_s *)userdata;
//    dlog_print(DLOG_DEBUG, LOG_TAG, "Peeking %d bytes", nbytes);
	if (nbytes > 0) {
		if (waitCycles > 9) {
			evas_object_hide(ad->waiting);
		}
		waitCycles = 0;
		audio_in_peek(handle, &buffer, &nbytes);
		short *buffend = ((char *)buffer) + nbytes;
		while (buffer < buffend) {
			data[activebuf][idx++] = *buffer++;
			if (idx >= BUFFSIZE)  {
				memcpy(data[1 - activebuf], data[activebuf] + BUFFSIZE / 2, BUFFSIZE / 2 * sizeof(float));
				evalbuf = data[activebuf];
				activebuf = 1 - activebuf;
				idx = BUFFSIZE / 2;
			}
		}
		audio_in_drop(handle);
		if (evalbuf != NULL)
			evaluate_audio(evalbuf, ad);
	}
}

