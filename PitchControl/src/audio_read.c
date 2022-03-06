/*
 * audio_read.cc
 *
 *  Created on: Mar 22, 2016
 *      Author: a552095
 */

#include <audio_io.h>
#include <stdio.h>
#include <system_settings.h>
#include <device/power.h>
#include <dlog.h>

#include "audio_read.h"
#include "audio_callback.h"

const static char *note_de[] = {
		"A", "A", "H", "C", "C", "D", "D", "E", "F", "F", "G", "G"
};

const static char *note_lat[] = {
		"la", "la", "si", "do", "do", "re", "re", "mi", "fa", "fa", "sol", "sol"
};

const static char *note[] = {
		"A", "A", "B", "C", "C", "D", "D", "E", "F", "F", "G", "G"
};

const static char *octave[MAXOCTAVE] = {
		"ctr,,", "ctr,", "gr.", "sm.", "sup'", "sup''", "sup'''", "sup''''"
};

const static char *octave_de[MAXOCTAVE] = {
		"ktr,,", "kontra", "gr.", "kl.", "sup'", "sup''", "sup'''", "sup''''"
};

const static char *octave_lat[MAXOCTAVE] = {
		"o-1", "o1", "o2", "o3", "o4", "o5", "o6", "o7"
};

audio_in_h input;
char isActive = 0;

void printError(appdata_s *ad, char *msg, int code) {
	char txt[80];
	sprintf(txt, "%s: %d", msg, code);
	evas_object_text_text_set(ad->freq, txt);
}

void activateAudioModule_locked(appdata_s *ad) {
	if (isActive) {
		dlog_print(DLOG_INFO, LOG_TAG, "Audio Module is already active");
		return;
	}
	dlog_print(DLOG_INFO, LOG_TAG, "Audio Record Start Requested");
	audio_io_error_e error_code;

	// Initialize the audio input device

	error_code = audio_in_create(SAMPLE_RATE, AUDIO_CHANNEL_MONO,
			AUDIO_SAMPLE_TYPE_S16_LE, &input);
	if (error_code) {
		printError(ad, "Fehler audio_in_create", error_code);
		return;
	}
	error_code = audio_in_set_stream_cb(input, io_stream_callback, ad);
	if (error_code) {
		printError(ad, "Fehler audio_in_set_stream", error_code);
		error_code = audio_in_destroy(input);
		return;
	}
	reset_data();
	error_code = audio_in_prepare(input);
	if (error_code) {
		printError(ad, "Fehler audio_in_prepare", error_code);
		error_code = audio_in_destroy(input);
		return;
	}
	isActive = 1;
	device_power_request_lock(POWER_LOCK_DISPLAY, 180000);
	char *locale;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	strncpy(language, locale, 2);
	language[2] = '\0';
	// Retrieve the current system language
	int notesize = 140, x = 40, y = 100;
	if (strcmp(language, "de") == 0) {
		noteName = note_de;
		octName = octave_de;
	} else if (strstr("fr|it|es|pt", language) != NULL) {
		noteName = note_lat;
		octName = octave_lat;
		notesize = 90;
		y = 70;
	} else {
		noteName = note;
		octName = octave;
	}
	evas_object_move(ad->note, ad->centerX - x, ad->centerY - y);
	evas_object_text_font_set(ad->note, "TizenSans:style=bold", notesize);
	dlog_print(DLOG_INFO, LOG_TAG, "Audio Record Start Executed");
	ad->timer = ecore_timer_add(0.08, displayNote, ad);
}

void activateAudioModule(appdata_s *ad) {
	eina_lock_take(&ad->mutex);
	activateAudioModule_locked(ad);
	eina_lock_release(&ad->mutex);
}

void deactivateAudioModule(appdata_s *ad) {
	eina_lock_take(&ad->mutex);
	if (!isActive) {
		dlog_print(DLOG_INFO, LOG_TAG, "Audio Module is already inactive");
		eina_lock_release(&ad->mutex);
		return;
	}
	dlog_print(DLOG_INFO, LOG_TAG, "Audio Record Stop Requested");
	int error_code;
	device_power_release_lock(POWER_LOCK_DISPLAY);
	error_code = audio_in_unprepare(input);
	error_code = audio_in_destroy(input);
	isActive = 0;
	dlog_print(DLOG_INFO, LOG_TAG, "Audio Record Stop Executed");
	ecore_timer_del(ad->timer);
	eina_lock_release(&ad->mutex);
}
