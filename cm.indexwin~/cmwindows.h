#ifndef _cmwindows_h
#define _cmwindows_h

void cm_hann(double *window, long *length);
void cm_hamming(double *window, long *length);
void cm_rectangular(double *window, long *length);
void cm_bartlett(double *window, long *length);
void cm_flattop(double *window, long *length);
void cm_gauss2(double *window, long *length);
void cm_gauss4(double *window, long *length);
void cm_gauss8(double *window, long *length);

#endif