/*
 Circuit Music Labs stereo utilities library file.
 Copyright (C) 2014  Matthias Müller - Circuit Music Labs
 Math and general principle from Boulanger & Lazzarini, "The Audio Programming Book".
 The MIT Press, 2010.
 
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
#include "cmstereo.h"

// constant power stereo function
void cm_panning(cm_panstruct *panstruct, double *pos) {
	const double piovr2 = 4.0 * atan(1.0) * 0.5;
	const double root2ovr2 = sqrt(2.0) * 0.5;
	panstruct->left = root2ovr2 * (cos((*pos * piovr2) * 0.5) - sin((*pos * piovr2) * 0.5));
	panstruct->right = root2ovr2 * (cos((*pos * piovr2) * 0.5) + sin((*pos * piovr2) * 0.5));
	return;
}
