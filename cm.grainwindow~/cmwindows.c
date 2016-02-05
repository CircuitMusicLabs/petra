/*
 Circuit Music Labs utilities library file.
 Copyright (C) 2014  Matthias Müller - Circuit Music Labs
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 circuit.music.labs@gmail.com
 
 */


#include <stdio.h>
#include <math.h>
#include "cmwindows.h"


// HANN WINDOW
void cm_hann(double *window, long *length) {
	int i;
	for (i = 0; i < *length; i++) {
		window[i] = 0.5 * (1.0 - cos((2.0 * M_PI * (double)i) / (*length - 1)));
	}
	return;
}

// RECTANGULAR WINDOW
void cm_rectangular(double *window, long *length) {
	int i;
	for (i = 0; i < *length; i++) {
		window[i] = 1;
	}
	return;
}

void cm_bartlett(double *window, long *length) {
	int i;
	for (i = 0; i < *length; i++) {
		if (i < (*length - 1) / 2) {
			window[i] = (2 * (double)i) / (double)(*length - 1);
		}
		else {
			window[i] = 2 - ((2 * (double)i) / (double)(*length - 1));
		}
	}
	return;
}
