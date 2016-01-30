/*
 cm.grainwindow~ - a granular synthesis external audio object for Max/MSP.
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

/************************************************************************************************************************/
/* INCLUDES                                                                                                             */
/************************************************************************************************************************/
#include "ext.h"
#include "z_dsp.h"
#include "buffer.h"
#include "ext_atomic.h"
#include "ext_obex.h"
#include "cmstereo.h" // for cm_pan
#include "cmutil.h" // for cm utility functions
#include "cmwindows.h" // for windowing function
#include <stdlib.h> // for arc4random_uniform
#define MAX_GRAINLENGTH 500 // max grain length in ms
#define MIN_GRAINLENGTH 1 // min grain length in ms
#define MAX_PITCH 10 // min pitch
#define MAX_GAIN 2.0  // max gain
#define ARGUMENTS 4 // constant number of arguments required for the external
#define MAXGRAINS 128 // maximum number of simultaneously playing grains
#define MIN_WINDOWLENGTH 16 // min window length in samples


/************************************************************************************************************************/
/* OBJECT STRUCTURE                                                                                                     */
/************************************************************************************************************************/
typedef struct _cmgrainwindow {
	t_pxobject obj;
	t_symbol *buffer_name; // sample buffer name
	t_buffer_ref *buffer; // sample buffer reference
	
	t_symbol *window_type; // window typedef
	int window_length; // window length
	short w_writeflag; // checkflag to see if window array is currently re-witten
	
	double m_sr; // system millisampling rate (samples per milliseconds = sr * 0.001)
	double startmin_float; // grain start min value received from float inlet
	double startmax_float; // grain start max value received from float inlet
	double lengthmin_float; // used to store the min length value received from float inlet
	double lengthmax_float; // used to store the max length value received from float inlet
	double pitchmin_float; // used to store the min pitch value received from float inlet
	double pitchmax_float; // used to store the max pitch value received from float inlet
	double panmin_float; // used to store the min pan value received from the float inlet
	double panmax_float; // used to store the max pan value received from the float inlet
	double gainmin_float; // used to store the min gain value received from the float inlet
	double gainmax_float; // used to store the max gain value received from the float inlet
	short connect_status[10]; // array for signal inlet connection statuses
	short *busy; // array used to store the flag if a grain is currently playing or not
	long *grainpos; // used to store the current playback position per grain
	long *start; // used to store the start position in the buffer for each grain
	long *t_length; // current grain length before pitch adjustment
	long *gr_length; // current grain length after pitch adjustment
	double *pan_left; // pan information for left channel for each grain
	double *pan_right; // pan information for right channel for each grain
	double *gain; // gain information for each grain
	double tr_prev; // trigger sample from previous signal vector (required to check if input ramp resets to zero)
	short grains_limit; // user defined maximum number of grains
	short grains_limit_old; // used to store the previous grains count limit when user changes the limit via the "limit" message
	short limit_modified; // checkflag to see if user changed grain limit through "limit" method
	short buffer_modified; // checkflag to see if buffer has been modified
	short grains_count; // currently playing grains
	void *grains_count_out; // outlet for number of currently playing grains (for debugging)
	t_atom_long attr_stereo; // attribute: number of channels to be played
	t_atom_long attr_winterp; // attribute: window interpolation on/off
	t_atom_long attr_sinterp; // attribute: window interpolation on/off
	t_atom_long attr_zero; // attribute: zero crossing trigger on/off
	
	double *window; // window array
	
} t_cmgrainwindow;


/************************************************************************************************************************/
/* STATIC DECLARATIONS                                                                                                  */
/************************************************************************************************************************/
static t_class *cmgrainwindow_class; // class pointer
static t_symbol *ps_buffer_modified, *ps_stereo;


/************************************************************************************************************************/
/* FUNCTION PROTOTYPES                                                                                                  */
/************************************************************************************************************************/
void *cmgrainwindow_new(t_symbol *s, long argc, t_atom *argv);
void cmgrainwindow_dsp64(t_cmgrainwindow *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void cmgrainwindow_perform64(t_cmgrainwindow *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void cmgrainwindow_assist(t_cmgrainwindow *x, void *b, long msg, long arg, char *dst);
void cmgrainwindow_free(t_cmgrainwindow *x);
void cmgrainwindow_float(t_cmgrainwindow *x, double f);
void cmgrainwindow_dblclick(t_cmgrainwindow *x);
t_max_err cmgrainwindow_notify(t_cmgrainwindow *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
void cmgrainwindow_set(t_cmgrainwindow *x, t_symbol *s, long ac, t_atom *av);
void cmgrainwindow_limit(t_cmgrainwindow *x, t_symbol *s, long ac, t_atom *av);

void cmgrainwindow_w_type(t_cmgrainwindow *x, t_symbol *s, long ac, t_atom *av);
void cmgrainwindow_w_length(t_cmgrainwindow *x, t_symbol *s, long ac, t_atom *av);

t_max_err cmgrainwindow_stereo_set(t_cmgrainwindow *x, t_object *attr, long argc, t_atom *argv);
t_max_err cmgrainwindow_winterp_set(t_cmgrainwindow *x, t_object *attr, long argc, t_atom *argv);
t_max_err cmgrainwindow_sinterp_set(t_cmgrainwindow *x, t_object *attr, long argc, t_atom *argv);
t_max_err cmgrainwindow_zero_set(t_cmgrainwindow *x, t_object *attr, long argc, t_atom *argv);

void cmgrainwindow_windowwrite(t_cmgrainwindow *x, t_symbol *s, int length);


/************************************************************************************************************************/
/* MAIN FUNCTION (INITIALIZATION ROUTINE)                                                                               */
/************************************************************************************************************************/
int C74_EXPORT main(void) {
	// Initialize the class - first argument: VERY important to match the name of the object in the procect settings!!!
	cmgrainwindow_class = class_new("cm.grainwindow~", (method)cmgrainwindow_new, (method)cmgrainwindow_free, sizeof(t_cmgrainwindow), 0, A_GIMME, 0);
	
	class_addmethod(cmgrainwindow_class, (method)cmgrainwindow_dsp64, 		"dsp64", 		A_CANT, 0);  // Bind the 64 bit dsp method
	class_addmethod(cmgrainwindow_class, (method)cmgrainwindow_assist, 		"assist", 		A_CANT, 0); // Bind the assist message
	class_addmethod(cmgrainwindow_class, (method)cmgrainwindow_float, 		"float", 		A_FLOAT, 0); // Bind the float message (allowing float input)
	class_addmethod(cmgrainwindow_class, (method)cmgrainwindow_dblclick, 	"dblclick",		A_CANT, 0); // Bind the double click message
	class_addmethod(cmgrainwindow_class, (method)cmgrainwindow_notify, 		"notify", 		A_CANT, 0); // Bind the notify message
	class_addmethod(cmgrainwindow_class, (method)cmgrainwindow_set, 		"set", 			A_GIMME, 0); // Bind the set message for user buffer set
	class_addmethod(cmgrainwindow_class, (method)cmgrainwindow_limit, 		"limit", 		A_GIMME, 0); // Bind the limit message
	class_addmethod(cmgrainwindow_class, (method)cmgrainwindow_limit, 		"w_type", 		A_GIMME, 0); // Bind the window type message
	class_addmethod(cmgrainwindow_class, (method)cmgrainwindow_limit, 		"w_length", 	A_GIMME, 0); // Bind the window length message
	
	CLASS_ATTR_ATOM_LONG(cmgrainwindow_class, "stereo", 0, t_cmgrainwindow, attr_stereo);
	CLASS_ATTR_ACCESSORS(cmgrainwindow_class, "stereo", (method)NULL, (method)cmgrainwindow_stereo_set);
	CLASS_ATTR_BASIC(cmgrainwindow_class, "stereo", 0);
	CLASS_ATTR_SAVE(cmgrainwindow_class, "stereo", 0);
	CLASS_ATTR_STYLE_LABEL(cmgrainwindow_class, "stereo", 0, "onoff", "Multichannel playback");
	
	CLASS_ATTR_ATOM_LONG(cmgrainwindow_class, "w_interp", 0, t_cmgrainwindow, attr_winterp);
	CLASS_ATTR_ACCESSORS(cmgrainwindow_class, "w_interp", (method)NULL, (method)cmgrainwindow_winterp_set);
	CLASS_ATTR_BASIC(cmgrainwindow_class, "w_interp", 0);
	CLASS_ATTR_SAVE(cmgrainwindow_class, "w_interp", 0);
	CLASS_ATTR_STYLE_LABEL(cmgrainwindow_class, "w_interp", 0, "onoff", "Window interpolation on/off");
	
	CLASS_ATTR_ATOM_LONG(cmgrainwindow_class, "s_interp", 0, t_cmgrainwindow, attr_sinterp);
	CLASS_ATTR_ACCESSORS(cmgrainwindow_class, "s_interp", (method)NULL, (method)cmgrainwindow_sinterp_set);
	CLASS_ATTR_BASIC(cmgrainwindow_class, "s_interp", 0);
	CLASS_ATTR_SAVE(cmgrainwindow_class, "s_interp", 0);
	CLASS_ATTR_STYLE_LABEL(cmgrainwindow_class, "s_interp", 0, "onoff", "Sample interpolation on/off");
	
	CLASS_ATTR_ATOM_LONG(cmgrainwindow_class, "zero", 0, t_cmgrainwindow, attr_zero);
	CLASS_ATTR_ACCESSORS(cmgrainwindow_class, "zero", (method)NULL, (method)cmgrainwindow_zero_set);
	CLASS_ATTR_BASIC(cmgrainwindow_class, "zero", 0);
	CLASS_ATTR_SAVE(cmgrainwindow_class, "zero", 0);
	CLASS_ATTR_STYLE_LABEL(cmgrainwindow_class, "zero", 0, "onoff", "Zero crossing trigger mode on/off");
	
	CLASS_ATTR_ORDER(cmgrainwindow_class, "stereo", 0, "1");
	CLASS_ATTR_ORDER(cmgrainwindow_class, "w_interp", 0, "2");
	CLASS_ATTR_ORDER(cmgrainwindow_class, "s_interp", 0, "3");
	
	class_dspinit(cmgrainwindow_class); // Add standard Max/MSP methods to your class
	class_register(CLASS_BOX, cmgrainwindow_class); // Register the class with Max
	ps_buffer_modified = gensym("buffer_modified"); // assign the buffer modified message to the static pointer created above
	ps_stereo = gensym("stereo");
	return 0;
}


/************************************************************************************************************************/
/* NEW INSTANCE ROUTINE                                                                                                 */
/************************************************************************************************************************/
void *cmgrainwindow_new(t_symbol *s, long argc, t_atom *argv) {
	t_cmgrainwindow *x = (t_cmgrainwindow *)object_alloc(cmgrainwindow_class); // create the object and allocate required memory
	dsp_setup((t_pxobject *)x, 11); // create 11 inlets
	
	if (argc < ARGUMENTS) {
		object_error((t_object *)x, "%d arguments required (sample buffer / window type / window length / voices)", ARGUMENTS);
		return NULL;
	}
	
	x->buffer_name = atom_getsymarg(0, argc, argv); // get user supplied argument for sample buffer
	x->window_type = atom_getsymarg(1, argc, argv); // get user supplied argument for window type
	x->window_length = atom_getsymarg(2, argc, argv); // get user supplied argument for window length
	x->grains_limit = atom_getintarg(3, argc, argv); // get user supplied argument for maximum grains
	
	// CHECK IF WINDOW TYPE ARGUMENT IS VALID
	if (x->window_type != gensym("hann") || x->window_type != gensym("rect")) {
		object_error((t_object *)x, "invalid window type");
		return NULL;
	}
	
	// CHECK IF WINDOW LENGTH ARGUMENT IS VALID
	if (x->window_length < MIN_WINDOWLENGTH) {
		object_error((t_object *)x, "window length must be greater than %d", MIN_WINDOWLENGTH);
		return NULL;
	}
	
	// HANDLE ATTRIBUTES
	object_attr_setlong(x, gensym("stereo"), 0); // initialize stereo attribute
	object_attr_setlong(x, gensym("w_interp"), 0); // initialize window interpolation attribute
	object_attr_setlong(x, gensym("s_interp"), 1); // initialize window interpolation attribute
	object_attr_setlong(x, gensym("zero"), 0); // initialize zero crossing attribute
	attr_args_process(x, argc, argv); // get attribute values if supplied as argument
	
	// CHECK IF USER SUPPLIED MAXIMUM GRAINS IS IN THE LEGAL RANGE (1 - MAXGRAINS)
	if (x->grains_limit < 1 || x->grains_limit > MAXGRAINS) {
		object_error((t_object *)x, "maximum grains allowed is %d", MAXGRAINS);
		return NULL;
	}
	
	// CREATE OUTLETS (OUTLETS ARE CREATED FROM RIGHT TO LEFT)
	x->grains_count_out = intout((t_object *)x); // create outlet for number of currently playing grains
	outlet_new((t_object *)x, "signal"); // right signal outlet
	outlet_new((t_object *)x, "signal"); // left signal outlet
	
	// GET SYSTEM SAMPLE RATE
	x->m_sr = sys_getsr() * 0.001; // get the current sample rate and write it into the object structure
	
	/************************************************************************************************************************/
	// ALLOCATE MEMORY FOR THE BUSY ARRAY
	x->busy = (short *)sysmem_newptrclear((MAXGRAINS) * sizeof(short *));
	if (x->busy == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE GRAINPOS ARRAY
	x->grainpos = (long *)sysmem_newptrclear((MAXGRAINS) * sizeof(long *));
	if (x->grainpos == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE START ARRAY
	x->start = (long *)sysmem_newptrclear((MAXGRAINS) * sizeof(long *));
	if (x->start == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE T_LENGTH ARRAY
	x->t_length = (long *)sysmem_newptrclear((MAXGRAINS) * sizeof(long *));
	if (x->t_length == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE GR_LENGTH ARRAY
	x->gr_length = (long *)sysmem_newptrclear((MAXGRAINS) * sizeof(long *));
	if (x->gr_length == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE PAN_LEFT ARRAY
	x->pan_left = (double *)sysmem_newptrclear((MAXGRAINS) * sizeof(double *));
	if (x->pan_left == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE PAN_RIGHT ARRAY
	x->pan_right = (double *)sysmem_newptrclear((MAXGRAINS) * sizeof(double *));
	if (x->pan_right == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE GAIN ARRAY
	x->gain = (double *)sysmem_newptrclear((MAXGRAINS) * sizeof(double *));
	if (x->gain == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE WINDOW ARRAY; this is new
	x->window = (double *)sysmem_newptrclear((x->window_length) * sizeof(double *));
	if (x->window == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	/************************************************************************************************************************/
	// INITIALIZE VALUES
	x->startmin_float = 0.0; // initialize float inlet value for current start min value
	x->startmax_float = 0.0; // initialize float inlet value for current start max value
	x->lengthmin_float = 150; // initialize float inlet value for min grain length
	x->lengthmax_float = 150; // initialize float inlet value for max grain length
	x->pitchmin_float = 1.0; // initialize inlet value for min pitch
	x->pitchmax_float = 1.0; // initialize inlet value for min pitch
	x->panmin_float = 0.0; // initialize value for min pan
	x->panmax_float = 0.0; // initialize value for max pan
	x->gainmin_float = 1.0; // initialize value for min gain
	x->gainmax_float = 1.0; // initialize value for max gain
	x->tr_prev = 0.0; // initialize value for previous trigger sample
	x->grains_count = 0; // initialize the grains count value
	x->grains_limit_old = 0; // initialize value for the routine when grains limit was modified
	x->limit_modified = 0; // initialize channel change flag
	x->buffer_modified = 0; // initialize buffer modified flag
	x->w_writeflag = 0; // initialize window write flag
	
	/************************************************************************************************************************/
	// BUFFER REFERENCES
	x->buffer = buffer_ref_new((t_object *)x, x->buffer_name); // write the buffer reference into the object structure
	
	// WRITE WINDOW INTO WINDOW ARRAY
	cmgrainwindow_windowwrite(x, x->window_type, x->window_length);
	
	return x;
}


/************************************************************************************************************************/
/* THE 64 BIT DSP METHOD                                                                                                */
/************************************************************************************************************************/
void cmgrainwindow_dsp64(t_cmgrainwindow *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags) {
	x->connect_status[0] = count[1]; // 2nd inlet: write connection flag into object structure (1 if signal connected)
	x->connect_status[1] = count[2]; // 3rd inlet: write connection flag into object structure (1 if signal connected)
	x->connect_status[2] = count[3]; // 4th inlet: write connection flag into object structure (1 if signal connected)
	x->connect_status[3] = count[4]; // 5th inlet: write connection flag into object structure (1 if signal connected)
	x->connect_status[4] = count[5]; // 6th inlet: write connection flag into object structure (1 if signal connected)
	x->connect_status[5] = count[6]; // 7th inlet: write connection flag into object structure (1 if signal connected)
	x->connect_status[6] = count[7]; // 8th inlet: write connection flag into object structure (1 if signal connected)
	x->connect_status[7] = count[8]; // 9th inlet: write connection flag into object structure (1 if signal connected)
	x->connect_status[8] = count[9]; // 10th inlet: write connection flag into object structure (1 if signal connected)
	x->connect_status[9] = count[10]; // 11th inlet: write connection flag into object structure (1 if signal connected)
	
	if (x->m_sr != samplerate * 0.001) { // check if sample rate stored in object structure is the same as the current project sample rate
		x->m_sr = samplerate * 0.001;
	}
	
	// CALL THE PERFORM ROUTINE
	//object_method(dsp64, gensym("dsp_add64"), x, cmgrainwindow_perform64, 0, NULL);
	dsp_add64(dsp64, (t_object*)x, (t_perfroutine64)cmgrainwindow_perform64, 0, NULL);
}


/************************************************************************************************************************/
/* THE 64 BIT PERFORM ROUTINE                                                                                           */
/************************************************************************************************************************/
void cmgrainwindow_perform64(t_cmgrainwindow *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam) {
	// VARIABLE DECLARATIONS
	short trigger = 0; // trigger occurred yes/no
	long i, limit; // for loop counterS
	long n = sampleframes; // number of samples per signal vector
	double tr_curr; // current trigger value
	double pan; // temporary random pan information
	double pitch; // temporary pitch for new grains
	double distance; // floating point index for reading from buffers
	long index; // truncated index for reading from buffers
	double b_read, w_read; // current sample read from the sample buffer and window array
	double outsample_left = 0.0; // temporary left output sample used for adding up all grain samples
	double outsample_right = 0.0; // temporary right output sample used for adding up all grain samples
	int slot = 0; // variable for the current slot in the arrays to write grain info to
	cm_panstruct panstruct; // struct for holding the calculated constant power left and right stereo values
	
	// OUTLETS
	t_double *out_left 	= (t_double *)outs[0]; // assign pointer to left output
	t_double *out_right = (t_double *)outs[1]; // assign pointer to right output
	
	// BUFFER VARIABLE DECLARATIONS
	t_buffer_obj *buffer = buffer_ref_getobject(x->buffer);
	float *b_sample = buffer_locksamples(buffer);
	long b_framecount; // number of frames in the sample buffer
	t_atom_long b_channelcount; // number of channels in the sample buffer
	
	float *w_sample = (float *)x->window;
	
	
	// BUFFER CHECKS
	if (!b_sample) { // if the sample buffer does not exist
		goto zero;
	}
	if (x->w_writeflag) { // if the window array is currently being rewritten
		goto zero;
	}
	
	// GET BUFFER INFORMATION
	b_framecount = buffer_getframecount(buffer); // get number of frames in the sample buffer
	b_channelcount = buffer_getchannelcount(buffer); // get number of channels in the sample buffer
	
	// GET INLET VALUES
	t_double *tr_sigin 	= (t_double *)ins[0]; // get trigger input signal from 1st inlet
	t_double startmin 	= x->connect_status[0]? *ins[1] * x->m_sr : x->startmin_float * x->m_sr; // get start min input signal from 2nd inlet
	t_double startmax 	= x->connect_status[1]? *ins[2] * x->m_sr : x->startmax_float * x->m_sr; // get start max input signal from 3rd inlet
	t_double lengthmin 	= x->connect_status[2]? *ins[3] * x->m_sr : x->lengthmin_float * x->m_sr; // get grain min length input signal from 4th inlet
	t_double lengthmax 	= x->connect_status[3]? *ins[4] * x->m_sr : x->lengthmax_float * x->m_sr; // get grain max length input signal from 5th inlet
	t_double pitchmin 	= x->connect_status[4]? *ins[5] : x->pitchmin_float; // get pitch min input signal from 6th inlet
	t_double pitchmax 	= x->connect_status[5]? *ins[6] : x->pitchmax_float; // get pitch max input signal from 7th inlet
	t_double panmin 	= x->connect_status[6]? *ins[7] : x->panmin_float; // get min pan input signal from 8th inlet
	t_double panmax 	= x->connect_status[7]? *ins[8] : x->panmax_float; // get max pan input signal from 9th inlet
	t_double gainmin 	= x->connect_status[8]? *ins[9] : x->gainmin_float; // get min gain input signal from 10th inlet
	t_double gainmax 	= x->connect_status[9]? *ins[10] : x->gainmax_float; // get max gain input signal from 10th inlet
	
	// DSP LOOP
	while (n--) {
		tr_curr = *tr_sigin++; // get current trigger value
		
		if (x->attr_zero) {
			if (tr_curr > 0.0 && x->tr_prev < 0.0) { // zero crossing from negative to positive
				trigger = 1;
			}
		}
		else {
			if ((x->tr_prev - tr_curr) > 0.9) {
				trigger = 1;
			}
		}
		
		if (x->buffer_modified) { // reset all playback information when any of the buffers was modified
			for (i = 0; i < MAXGRAINS; i++) {
				x->busy[i] = 0;
			}
			x->grains_count = 0;
			x->buffer_modified = 0;
		}
		/************************************************************************************************************************/
		// IN CASE OF TRIGGER, LIMIT NOT MODIFIED AND GRAINS COUNT IN THE LEGAL RANGE (AVAILABLE SLOTS)
		if (trigger && x->grains_count < x->grains_limit && !x->limit_modified) { // based on zero crossing --> when ramp from 0-1 restarts.
			trigger = 0; // reset trigger
			x->grains_count++; // increment grains_count
			// FIND A FREE SLOT FOR THE NEW GRAIN
			i = 0;
			while (i < x->grains_limit) {
				if (!x->busy[i]) {
					x->busy[i] = 1;
					slot = i;
					break;
				}
				i++;
			}
			/************************************************************************************************************************/
			// GET RANDOM START POSITION
			if (startmin != startmax) { // only call random function when min and max values are not the same!
				x->start[slot] = (long)cm_random(&startmin, &startmax);
			}
			else {
				x->start[slot] = startmin;
			}
			/************************************************************************************************************************/
			// GET RANDOM LENGTH
			if (lengthmin != lengthmax) { // only call random function when min and max values are not the same!
				x->t_length[slot] = (long)cm_random(&lengthmin, &lengthmax);
			}
			else {
				x->t_length[slot] = lengthmin;
			}
			// CHECK IF THE VALUE FOR PERCEPTIBLE GRAIN LENGTH IS LEGAL
			if (x->t_length[slot] > MAX_GRAINLENGTH * x->m_sr) { // if grain length is larger than the max grain length
				x->t_length[slot] = MAX_GRAINLENGTH * x->m_sr; // set grain length to max grain length
			}
			else if (x->t_length[slot] < MIN_GRAINLENGTH * x->m_sr) { // if grain length is samller than the min grain length
				x->t_length[slot] = MIN_GRAINLENGTH * x->m_sr; // set grain length to min grain length
			}
			/************************************************************************************************************************/
			// GET RANDOM PAN
			if (panmin != panmax) { // only call random function when min and max values are not the same!
				pan = cm_random(&panmin, &panmax);
			}
			else {
				pan = panmin;
			}
			// SOME SANITY TESTING
			if (pan < -1.0) {
				pan = -1.0;
			}
			if (pan > 1.0) {
				pan = 1.0;
			}
			cm_panning(&panstruct, &pan); // calculate pan values in panstruct
			x->pan_left[slot] = panstruct.left;
			x->pan_right[slot] = panstruct.right;
			/************************************************************************************************************************/
			// GET RANDOM PITCH
			if (pitchmin != pitchmax) { // only call random function when min and max values are not the same!
				pitch = cm_random(&pitchmin, &pitchmax);
			}
			else {
				pitch = pitchmin;
			}
			// CHECK IF THE PITCH VALUE IS LEGAL
			if (pitch < 0.001) {
				pitch = 0.001;
			}
			if (pitch > MAX_PITCH) {
				pitch = MAX_PITCH;
			}
			/************************************************************************************************************************/
			// GET RANDOM GAIN
			if (gainmin != gainmax) {
				x->gain[slot] = cm_random(&gainmin, &gainmax);
			}
			else {
				x->gain[slot] = gainmin;
			}
			// CHECK IF THE GAIN VALUE IS LEGAL
			if (x->gain[slot] < 0.0) {
				x->gain[slot] = 0.0;
			}
			if (x->gain[slot] > MAX_GAIN) {
				x->gain[slot] = MAX_GAIN;
			}
			/************************************************************************************************************************/
			// CALCULATE THE ACTUAL GRAIN LENGTH (SAMPLES) ACCORDING TO PITCH
			x->gr_length[slot] = x->t_length[slot] * pitch;
			// CHECK THAT GRAIN LENGTH IS NOT LARGER THAN SIZE OF BUFFER
			if (x->gr_length[slot] > b_framecount) {
				x->gr_length[slot] = b_framecount;
			}
			/************************************************************************************************************************/
			// CHECK IF START POSITION IS LEGAL ACCORDING TO GRAINzLENGTH (SAMPLES) AND BUFFER SIZE
			if (x->start[slot] > b_framecount - x->gr_length[slot]) {
				x->start[slot] = b_framecount - x->gr_length[slot];
			}
			if (x->start[slot] < 0) {
				x->start[slot] = 0;
			}
		}
		/************************************************************************************************************************/
		// CONTINUE WITH THE PLAYBACK ROUTINE
		if (x->grains_count == 0) { // if grains count is zero, there is no playback to be calculated
			*out_left++ = 0.0;
			*out_right++ = 0.0;
		}
		else if (!b_sample) {
			*out_left++ = 0.0;
			*out_right++ = 0.0;
		}
		else if (x->w_writeflag) {
			*out_left++ = 0.0;
			*out_right++ = 0.0;
		}
		else {
			if (x->limit_modified) {
				limit = x->grains_limit_old;
			}
			else {
				limit = x->grains_limit;
			}
			for (i = 0; i < limit; i++) {
				if (x->busy[i]) { // if the current slot contains grain playback information
					// GET WINDOW SAMPLE FROM WINDOW BUFFER
					if (x->attr_winterp) {
						distance = ((double)x->grainpos[i] / (double)x->t_length[i]) * (double)x->window_length;
						w_read = cm_lininterp(distance, w_sample, 1, 0);
					}
					else {
						index = (long)(((double)x->grainpos[i] / (double)x->t_length[i]) * (double)x->window_length);
						w_read = w_sample[index];
					}
					// GET GRAIN SAMPLE FROM SAMPLE BUFFER
					distance = x->start[i] + (((double)x->grainpos[i]++ / (double)x->t_length[i]) * (double)x->gr_length[i]);
					
					if (b_channelcount > 1 && x->attr_stereo) { // if more than one channel
						if (x->attr_sinterp) {
							outsample_left += ((cm_lininterp(distance, b_sample, b_channelcount, 0) * w_read) * x->pan_left[i]) * x->gain[i]; // get interpolated sample
							outsample_right += ((cm_lininterp(distance, b_sample, b_channelcount, 1) * w_read) * x->pan_right[i]) * x->gain[i];
						}
						else {
							outsample_left += ((b_sample[(long)distance * b_channelcount] * w_read) * x->pan_left[i]) * x->gain[i];
							outsample_right += ((b_sample[((long)distance * b_channelcount) + 1] * w_read) * x->pan_right[i]) * x->gain[i];
						}
					}
					else {
						if (x->attr_sinterp) {
							b_read = cm_lininterp(distance, b_sample, b_channelcount, 0) * w_read; // get interpolated sample
							outsample_left += (b_read * x->pan_left[i]) * x->gain[i];
							outsample_right += (b_read * x->pan_right[i]) * x->gain[i];
						}
						else {
							outsample_left += ((b_sample[(long)distance * b_channelcount] * w_read) * x->pan_left[i]) * x->gain[i];
							outsample_right += ((b_sample[(long)distance * b_channelcount] * w_read) * x->pan_right[i]) * x->gain[i];
						}
					}
					if (x->grainpos[i] == x->t_length[i]) { // if current grain has reached the end position
						x->grainpos[i] = 0; // reset parameters for overwrite
						x->busy[i] = 0;
						x->grains_count--;
						if (x->grains_count < 0) {
							x->grains_count = 0;
						}
					}
				}
			}
			*out_left++ = outsample_left; // write added sample values to left output vector
			*out_right++ = outsample_right; // write added sample values to right output vector
		}
		// CHECK IF GRAINS COUNT IS ZERO, THEN RESET LIMIT_MODIFIED CHECKFLAG
		if (x->grains_count == 0) {
			x->limit_modified = 0; // reset limit modified checkflag
		}
		
		/************************************************************************************************************************/
		x->tr_prev = tr_curr; // store current trigger value in object structure
		outsample_left = 0.0;
		outsample_right = 0.0;
	}
	
	/************************************************************************************************************************/
	// STORE UPDATED RUNNING VALUES INTO THE OBJECT STRUCTURE
	buffer_unlocksamples(buffer);
	outlet_int(x->grains_count_out, x->grains_count); // send number of currently playing grains to the outlet
	return;
	
zero:
	while (n--) {
		*out_left++ = 0.0;
		*out_right++ = 0.0;
	}
	buffer_unlocksamples(buffer);
	return; // THIS RETURN WAS MISSING FOR A LONG, LONG TIME. MAYBE THIS HELPS WITH STABILITY!?
}


/************************************************************************************************************************/
/* ASSIST METHOD FOR INLET AND OUTLET ANNOTATION                                                                        */
/************************************************************************************************************************/
void cmgrainwindow_assist(t_cmgrainwindow *x, void *b, long msg, long arg, char *dst) {
	if (msg == ASSIST_INLET) {
		switch (arg) {
			case 0:
				snprintf_zero(dst, 256, "(signal) trigger in");
				break;
			case 1:
				snprintf_zero(dst, 256, "(signal/float) start min");
				break;
			case 2:
				snprintf_zero(dst, 256, "(signal/float) start max");
				break;
			case 3:
				snprintf_zero(dst, 256, "(signal/float) min grain length");
				break;
			case 4:
				snprintf_zero(dst, 256, "(signal/float) max grain length");
				break;
			case 5:
				snprintf_zero(dst, 256, "(signal/float) pitch min");
				break;
			case 6:
				snprintf_zero(dst, 256, "(signal/float) pitch max");
				break;
			case 7:
				snprintf_zero(dst, 256, "(signal/float) pan min");
				break;
			case 8:
				snprintf_zero(dst, 256, "(signal/float) pan max");
				break;
			case 9:
				snprintf_zero(dst, 256, "(signal/float) gain min");
				break;
			case 10:
				snprintf_zero(dst, 256, "(signal/float) gain max");
				break;
		}
	}
	else if (msg == ASSIST_OUTLET) {
		switch (arg) {
			case 0:
				snprintf_zero(dst, 256, "(signal) output ch1");
				break;
			case 1:
				snprintf_zero(dst, 256, "(signal) output ch2");
				break;
			case 2:
				snprintf_zero(dst, 256, "(int) current grain count");
				break;
		}
	}
}


/************************************************************************************************************************/
/* FREE FUNCTION                                                                                                        */
/************************************************************************************************************************/
void cmgrainwindow_free(t_cmgrainwindow *x) {
	dsp_free((t_pxobject *)x); // free memory allocated for the object
	object_free(x->buffer); // free the buffer reference
	
	sysmem_freeptr(x->window); // free memory allocated to the window array
	
	sysmem_freeptr(x->busy); // free memory allocated to the busy array
	sysmem_freeptr(x->grainpos); // free memory allocated to the grainpos array
	sysmem_freeptr(x->start); // free memory allocated to the start array
	sysmem_freeptr(x->t_length); // free memory allocated to the t_length array
	sysmem_freeptr(x->gr_length); // free memory allocated to the t_length array
	sysmem_freeptr(x->pan_left); // free memory allocated to the pan_left array
	sysmem_freeptr(x->pan_right); // free memory allocated to the pan_right array
	sysmem_freeptr(x->gain); // free memory allocated to the gain array
}

/************************************************************************************************************************/
/* FLOAT METHOD FOR FLOAT INLET SUPPORT                                                                                 */
/************************************************************************************************************************/
void cmgrainwindow_float(t_cmgrainwindow *x, double f) {
	double dump;
	int inlet = ((t_pxobject*)x)->z_in; // get info as to which inlet was addressed (stored in the z_in component of the object structure
	switch (inlet) {
		case 1: // first inlet
			if (f < 0.0) {
				dump = f;
			}
			else {
				x->startmin_float = f;
			}
			break;
		case 2: // second inlet
			if (f < 0.0) {
				dump = f;
			}
			else {
				x->startmax_float = f;
			}
			break;
		case 3: // 4th inlet
			if (f < MIN_GRAINLENGTH) {
				dump = f;
			}
			else if (f > MAX_GRAINLENGTH) {
				dump = f;
			}
			else {
				x->lengthmin_float = f;
			}
			break;
		case 4: // 5th inlet
			if (f < MIN_GRAINLENGTH) {
				dump = f;
			}
			else if (f > MAX_GRAINLENGTH) {
				dump = f;
			}
			else {
				x->lengthmax_float = f;
			}
			break;
		case 5: // 6th inlet
			if (f <= 0.0) {
				dump = f;
			}
			else if (f > MAX_PITCH) {
				dump = f;
			}
			else {
				x->pitchmin_float = f;
			}
			break;
		case 6: // 7th inlet
			if (f <= 0.0) {
				dump = f;
			}
			else if (f > MAX_PITCH) {
				dump = f;
			}
			else {
				x->pitchmax_float = f;
			}
			break;
		case 7:
			if (f < -1.0 || f > 1.0) {
				dump = f;
			}
			else {
				x->panmin_float = f;
			}
			break;
		case 8:
			if (f < -1.0 || f > 1.0) {
				dump = f;
			}
			else {
				x->panmax_float = f;
			}
			break;
		case 9:
			if (f < 0.0 || f > MAX_GAIN) {
				dump = f;
			}
			else {
				x->gainmin_float = f;
			}
			break;
		case 10:
			if (f < 0.0 || f > MAX_GAIN) {
				dump = f;
			}
			else {
				x->gainmax_float = f;
			}
			break;
	}
}


/************************************************************************************************************************/
/* DOUBLE CLICK METHOD FOR VIEWING BUFFER CONTENT                                                                       */
/************************************************************************************************************************/
void cmgrainwindow_dblclick(t_cmgrainwindow *x) {
	buffer_view(buffer_ref_getobject(x->buffer));
}


/************************************************************************************************************************/
/* NOTIFY METHOD FOR THE BUFFER REFERENCES                                                                              */
/************************************************************************************************************************/
t_max_err cmgrainwindow_notify(t_cmgrainwindow *x, t_symbol *s, t_symbol *msg, void *sender, void *data) {
	if (msg == ps_buffer_modified) {
		x->buffer_modified = 1;
	}
	return buffer_ref_notify(x->buffer, s, msg, sender, data); // return with the calling buffer
}


/************************************************************************************************************************/
/* THE BUFFER SET METHOD                                                                                                */
/************************************************************************************************************************/
void cmgrainwindow_set(t_cmgrainwindow *x, t_symbol *s, long ac, t_atom *av) {
	if (ac == 1) {
		x->buffer_modified = 1;
		x->buffer_name = atom_getsym(av); // write buffer name into object structure
		buffer_ref_set(x->buffer, x->buffer_name);
		if (buffer_getchannelcount((t_object *)(buffer_ref_getobject(x->buffer))) > 2) {
			object_error((t_object *)x, "referenced sample buffer has more than 2 channels. using channels 1 and 2.");
		}
	}
	else {
		object_error((t_object *)x, "argument required (sample buffer name)");
	}
}






/************************************************************************************************************************/
/* THE WINDOW TYPE SET METHOD                                                                                           */
/************************************************************************************************************************/
void cmgrainwindow_w_type(t_cmgrainwindow *x, t_symbol *s, long ac, t_atom *av) {
	if (ac == 1) {
		if (x->w_writeflag == 0) { // only if the window array is not currently being rewritten
			// CHECK IF WINDOW TYPE ARGUMENT IS VALID
			if (atom_getsym(av) != gensym("hann") || atom_getsym(av) != gensym("rect")) {
				object_error((t_object *)x, "invalid window type");
			}
			else {
				x->window_type = atom_getsym(av); // write window type into object structure
				cmgrainwindow_windowwrite(x, x->window_type, x->window_length); // write window into window array
			}
		}
	}
	else {
		object_error((t_object *)x, "argument required (window type)");
	}
}



/************************************************************************************************************************/
/* THE WINDOW LENGTH SET METHOD                                                                                         */
/************************************************************************************************************************/
void cmgrainwindow_w_length(t_cmgrainwindow *x, t_symbol *s, long ac, t_atom *av) {
	int arg = atom_getlong(av);
	if (ac == 1) {
		// CHECK IF WINDOW LENGTH ARGUMENT IS VALID
		if (arg < MIN_WINDOWLENGTH) {
			object_error((t_object *)x, "window length must be greater than %d", MIN_WINDOWLENGTH);
		}
		else if (x->w_writeflag == 0) { // only if the window array is not currently being rewritten
			x->window_length = arg; // write window length into object structure
			x->w_writeflag = 1;
			x->window = (double *)sysmem_resizeptrclear(x->window, x->window_length * sizeof(double *)); // resize and clear window array
			x->w_writeflag = 0;
			cmgrainwindow_windowwrite(x, x->window_type, x->window_length); // write window into window array
		}
	}
	else {
		object_error((t_object *)x, "argument required (window length)");
	}
}




/************************************************************************************************************************/
/* THE GRAINS LIMIT METHOD                                                                                              */
/************************************************************************************************************************/
void cmgrainwindow_limit(t_cmgrainwindow *x, t_symbol *s, long ac, t_atom *av) {
	long arg;
	arg = atom_getlong(av);
	if (arg < 1 || arg > MAXGRAINS) {
		object_error((t_object *)x, "value must be in the range 1 - %d", MAXGRAINS);
	}
	else {
		x->grains_limit_old = x->grains_limit;
		x->grains_limit = arg;
		x->limit_modified = 1;
	}
}


/************************************************************************************************************************/
/* THE STEREO ATTRIBUTE SET METHOD                                                                                      */
/************************************************************************************************************************/
t_max_err cmgrainwindow_stereo_set(t_cmgrainwindow *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_stereo = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}


/************************************************************************************************************************/
/* THE WINDOW INTERPOLATION ATTRIBUTE SET METHOD                                                                        */
/************************************************************************************************************************/
t_max_err cmgrainwindow_winterp_set(t_cmgrainwindow *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_winterp = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}


/************************************************************************************************************************/
/* THE SAMPLE INTERPOLATION ATTRIBUTE SET METHOD                                                                        */
/************************************************************************************************************************/
t_max_err cmgrainwindow_sinterp_set(t_cmgrainwindow *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_sinterp = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}


/************************************************************************************************************************/
/* THE ZERO CROSSING ATTRIBUTE SET METHOD                                                                               */
/************************************************************************************************************************/
t_max_err cmgrainwindow_zero_set(t_cmgrainwindow *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_zero = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}

/************************************************************************************************************************/
/* THE WINDOW_WRITE FUNCTION                                                                                            */
/************************************************************************************************************************/
void cmgrainwindow_windowwrite(t_cmgrainwindow *x, t_symbol *type, int length) {
	x->w_writeflag = 1;
	if (type == gensym("hann")) {
		cm_hann(x->window, length);
	}
	else if (type == gensym("rect")) {
		cm_rectangular(x->window, length);
	}
	x->w_writeflag = 0;
	return;
}







