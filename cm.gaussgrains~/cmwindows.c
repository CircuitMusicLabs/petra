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


double cm_gauss(long pos, long length, double alpha) {
	double n;
	double N = length - 1;
	double stdev = N / (2 * (alpha));
	n = pos - N / 2;
	return exp(-0.5 * pow((n / stdev), 2));
}
