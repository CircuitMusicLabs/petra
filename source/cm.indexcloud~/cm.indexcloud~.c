/*
 cm.indexcloud~ - a granular synthesis external audio object for Max/MSP.
 Copyright (C) 2012 - 2017  Matthias W. Müller - circuit.music.labs

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

 info@circuitmusiclabs.com

 */

/************************************************************************************************************************/
/* INCLUDES                                                                                                             */
/************************************************************************************************************************/
#include "ext.h"
#include "z_dsp.h"
#include "buffer.h"
#include "ext_atomic.h"
#include "ext_obex.h"
#include <stdlib.h> // for arc4random_uniform
#include <math.h> // for stereo functions
#define MAX_GRAINLENGTH 500 // max grain length in ms
#define MIN_GRAINLENGTH 1 // min grain length in ms
#define MIN_PITCH 0.001 // min pitch
#define MAX_PITCH 10 // max pitch
#define MIN_PAN -1.0 // min pan
#define MAX_PAN 1.0 // max pan
#define MIN_GAIN 0.0 // min gain
#define MAX_GAIN 2.0  // max gain
#define ARGUMENTS 4 // constant number of arguments required for the external
#define MAXGRAINS 512 // maximum number of simultaneously playing grains
#define MIN_WINDOWLENGTH 16 // min window length in samples
#define MAX_WININDEX 7 // max object attribute value for window type
#define FLOAT_INLETS 10 // number of object float inlets
#define RANDMAX 10000

#ifdef WIN_VERSION
	#define M_PI 3.14159265358979323846264338327950288
#endif


/************************************************************************************************************************/
/* GRAIN MEMORY STORAGE                                                                                                 */
/************************************************************************************************************************/
typedef struct cmgrainmem {
	double *left;
	double *right;
	long length;
	long pos;
	short busy; // used to store the flag if a grain is currently playing or not
} cm_grainmem;


/************************************************************************************************************************/
/* OBJECT STRUCTURE                                                                                                     */
/************************************************************************************************************************/
typedef struct _cmindexcloud {
	t_pxobject obj;
	t_symbol *buffer_name; // sample buffer name
	t_buffer_ref *buffer; // sample buffer reference
	double *window; // window array
	long window_type; // window typedef
	long window_length; // window length
	short w_writeflag; // checkflag to see if window array is currently re-witten
	short w_window_modified; // checkflag to mark window changed
	double m_sr; // system millisampling rate (samples per milliseconds = sr * 0.001)
	short connect_status[FLOAT_INLETS]; // array for signal inlet connection statuses
	double *object_inlets; // array to store the incoming values coming from the object inlets
	double *grain_params; // array to store the processed values coming from the object inlets
	double *randomized; // array to store the randomized grain values
	double *testvalues; // array for storing the grain parameter test values (sanity testing)
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
	double piovr2; // pi over two for panning function
	double root2ovr2; // root of 2 over two for panning function
	short bang_trigger; // trigger received from bang method
	cm_grainmem *grainmem; // struct array for grain playback information
} t_cmindexcloud;


/************************************************************************************************************************/
/* STEREO STRUCTURE                                                                                                     */
/************************************************************************************************************************/
typedef struct cmpanner {
	double left;
	double right;
} cm_panstruct;


/************************************************************************************************************************/
/* STATIC DECLARATIONS                                                                                                  */
/************************************************************************************************************************/
static t_class *cmindexcloud_class; // class pointer
static t_symbol *ps_buffer_modified, *ps_stereo;


/************************************************************************************************************************/
/* FUNCTION PROTOTYPES                                                                                                  */
/************************************************************************************************************************/
void *cmindexcloud_new(t_symbol *s, long argc, t_atom *argv);
void cmindexcloud_dsp64(t_cmindexcloud *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void cmindexcloud_perform64(t_cmindexcloud *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void cmindexcloud_assist(t_cmindexcloud *x, void *b, long msg, long arg, char *dst);
void cmindexcloud_free(t_cmindexcloud *x);
void cmindexcloud_float(t_cmindexcloud *x, double f);
void cmindexcloud_dblclick(t_cmindexcloud *x);
t_max_err cmindexcloud_notify(t_cmindexcloud *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
void cmindexcloud_set(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av);
void cmindexcloud_limit(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av);
void cmindexcloud_bang(t_cmindexcloud *x);

void cmindexcloud_w_type(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av);
void cmindexcloud_w_length(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av);

t_max_err cmindexcloud_stereo_set(t_cmindexcloud *x, t_object *attr, long argc, t_atom *argv);
t_max_err cmindexcloud_winterp_set(t_cmindexcloud *x, t_object *attr, long argc, t_atom *argv);
t_max_err cmindexcloud_sinterp_set(t_cmindexcloud *x, t_object *attr, long argc, t_atom *argv);
t_max_err cmindexcloud_zero_set(t_cmindexcloud *x, t_object *attr, long argc, t_atom *argv);

void cmindexcloud_windowwrite(t_cmindexcloud *x);

// PANNING FUNCTION
void cm_panning(cm_panstruct *panstruct, double *pos, t_cmindexcloud *x);
// RANDOM NUMBER GENERATOR
double cm_random(double *min, double *max);
// LINEAR INTERPOLATION FUNCTIONS
double cm_lininterp(double distance, float *b_sample, t_atom_long b_channelcount, short channel);
double cm_lininterpwin(double distance, double *buffer, t_atom_long b_channelcount, short channel);
// WINDOW FUNCTIONS
void cm_hann(double *window, long *length);
void cm_hamming(double *window, long *length);
void cm_rectangular(double *window, long *length);
void cm_bartlett(double *window, long *length);
void cm_flattop(double *window, long *length);
void cm_gauss2(double *window, long *length);
void cm_gauss4(double *window, long *length);
void cm_gauss8(double *window, long *length);


/************************************************************************************************************************/
/* MAIN FUNCTION (INITIALIZATION ROUTINE)                                                                               */
/************************************************************************************************************************/
void ext_main(void *r) {
	// Initialize the class - first argument: VERY important to match the name of the object in the procect settings!!!
	cmindexcloud_class = class_new("cm.indexcloud~", (method)cmindexcloud_new, (method)cmindexcloud_free, sizeof(t_cmindexcloud), 0, A_GIMME, 0);

	class_addmethod(cmindexcloud_class, (method)cmindexcloud_dsp64, 		"dsp64", 		A_CANT, 0);  // Bind the 64 bit dsp method
	class_addmethod(cmindexcloud_class, (method)cmindexcloud_assist, 		"assist", 		A_CANT, 0); // Bind the assist message
	class_addmethod(cmindexcloud_class, (method)cmindexcloud_float, 		"float", 		A_FLOAT, 0); // Bind the float message (allowing float input)
	class_addmethod(cmindexcloud_class, (method)cmindexcloud_dblclick,		"dblclick",		A_CANT, 0); // Bind the double click message
	class_addmethod(cmindexcloud_class, (method)cmindexcloud_notify, 		"notify", 		A_CANT, 0); // Bind the notify message
	class_addmethod(cmindexcloud_class, (method)cmindexcloud_set,			"set", 			A_GIMME, 0); // Bind the set message for user buffer set
	class_addmethod(cmindexcloud_class, (method)cmindexcloud_limit, 		"limit", 		A_GIMME, 0); // Bind the limit message
	class_addmethod(cmindexcloud_class, (method)cmindexcloud_w_type,		"w_type", 		A_GIMME, 0); // Bind the window type message
	class_addmethod(cmindexcloud_class, (method)cmindexcloud_w_length,		"w_length", 	A_GIMME, 0); // Bind the window length message
	class_addmethod(cmindexcloud_class, (method)cmindexcloud_bang,			"bang",			0);


	CLASS_ATTR_ATOM_LONG(cmindexcloud_class, "stereo", 0, t_cmindexcloud, attr_stereo);
	CLASS_ATTR_ACCESSORS(cmindexcloud_class, "stereo", (method)NULL, (method)cmindexcloud_stereo_set);
	CLASS_ATTR_BASIC(cmindexcloud_class, "stereo", 0);
	CLASS_ATTR_SAVE(cmindexcloud_class, "stereo", 0);
	CLASS_ATTR_STYLE_LABEL(cmindexcloud_class, "stereo", 0, "onoff", "Multichannel playback");

	CLASS_ATTR_ATOM_LONG(cmindexcloud_class, "w_interp", 0, t_cmindexcloud, attr_winterp);
	CLASS_ATTR_ACCESSORS(cmindexcloud_class, "w_interp", (method)NULL, (method)cmindexcloud_winterp_set);
	CLASS_ATTR_BASIC(cmindexcloud_class, "w_interp", 0);
	CLASS_ATTR_SAVE(cmindexcloud_class, "w_interp", 0);
	CLASS_ATTR_STYLE_LABEL(cmindexcloud_class, "w_interp", 0, "onoff", "Window interpolation on/off");

	CLASS_ATTR_ATOM_LONG(cmindexcloud_class, "s_interp", 0, t_cmindexcloud, attr_sinterp);
	CLASS_ATTR_ACCESSORS(cmindexcloud_class, "s_interp", (method)NULL, (method)cmindexcloud_sinterp_set);
	CLASS_ATTR_BASIC(cmindexcloud_class, "s_interp", 0);
	CLASS_ATTR_SAVE(cmindexcloud_class, "s_interp", 0);
	CLASS_ATTR_STYLE_LABEL(cmindexcloud_class, "s_interp", 0, "onoff", "Sample interpolation on/off");

	CLASS_ATTR_ATOM_LONG(cmindexcloud_class, "zero", 0, t_cmindexcloud, attr_zero);
	CLASS_ATTR_ACCESSORS(cmindexcloud_class, "zero", (method)NULL, (method)cmindexcloud_zero_set);
	CLASS_ATTR_BASIC(cmindexcloud_class, "zero", 0);
	CLASS_ATTR_SAVE(cmindexcloud_class, "zero", 0);
	CLASS_ATTR_STYLE_LABEL(cmindexcloud_class, "zero", 0, "onoff", "Zero crossing trigger mode on/off");

	CLASS_ATTR_ORDER(cmindexcloud_class, "stereo", 0, "1");
	CLASS_ATTR_ORDER(cmindexcloud_class, "w_interp", 0, "2");
	CLASS_ATTR_ORDER(cmindexcloud_class, "s_interp", 0, "3");
	CLASS_ATTR_ORDER(cmindexcloud_class, "zero", 0, "4");

	class_dspinit(cmindexcloud_class); // Add standard Max/MSP methods to your class
	class_register(CLASS_BOX, cmindexcloud_class); // Register the class with Max
	ps_buffer_modified = gensym("buffer_modified"); // assign the buffer modified message to the static pointer created above
	ps_stereo = gensym("stereo");
}


/************************************************************************************************************************/
/* NEW INSTANCE ROUTINE                                                                                                 */
/************************************************************************************************************************/
void *cmindexcloud_new(t_symbol *s, long argc, t_atom *argv) {
	long i;
	t_cmindexcloud *x = (t_cmindexcloud *)object_alloc(cmindexcloud_class); // create the object and allocate required memory
	dsp_setup((t_pxobject *)x, 11); // create 11 inlets

	if (argc < ARGUMENTS) {
		object_error((t_object *)x, "%d arguments required (sample buffer / window type / window length / voices)", ARGUMENTS);
		return NULL;
	}

	x->buffer_name = atom_getsymarg(0, argc, argv); // get user supplied argument for sample buffer
	x->window_type = atom_getintarg(1, argc, argv); // get user supplied argument for window type
	x->window_length = atom_getintarg(2, argc, argv); // get user supplied argument for window length
	x->grains_limit = atom_getintarg(3, argc, argv); // get user supplied argument for maximum grains

	// CHECK IF WINDOW TYPE ARGUMENT IS VALID
	if (x->window_type < 0 || x->window_type > MAX_WININDEX) {
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
	// ALLOCATE MEMORY FOR THE WINDOW ARRAY; this is new
	x->window = (double *)sysmem_newptrclear((x->window_length) * sizeof(double));
	if (x->window == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}

	// ALLOCATE MEMORY FOR THE OBJET FLOAT_INLETS ARRAY
	x->object_inlets = (double *)sysmem_newptrclear((FLOAT_INLETS) * sizeof(double));
	if (x->object_inlets == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}

	// ALLOCATE MEMORY FOR THE GRAIN PARAMETERS ARRAY
	x->grain_params = (double *)sysmem_newptrclear((FLOAT_INLETS) * sizeof(double));
	if (x->grain_params == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}

	// ALLOCATE MEMORY FOR THE GRAIN PARAMETERS ARRAY
	x->randomized = (double *)sysmem_newptrclear((FLOAT_INLETS / 2) * sizeof(double));
	if (x->randomized == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}

	// ALLOCATE MEMORY FOR THE TEST VALUES ARRAY
	x->testvalues = (double *)sysmem_newptrclear((FLOAT_INLETS) * sizeof(double));
	if (x->testvalues == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE GRAINMEM ARRAY
	x->grainmem = (cm_grainmem *)sysmem_newptrclear((MAXGRAINS) * sizeof(cm_grainmem));
	if (x->grainmem == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE GRAIN ARRAY IN EACH MEMBER OF THE GRAINMEM STRUCT
	for (i = 0; i < MAXGRAINS; i++) {
		x->grainmem[i].left = (double *)sysmem_newptrclear(((MAX_GRAINLENGTH * x->m_sr) * MAX_PITCH) * sizeof(double));
		if (x->grainmem[i].left == NULL) {
			object_error((t_object *)x, "out of memory");
			return NULL;
		}
		x->grainmem[i].right = (double *)sysmem_newptrclear(((MAX_GRAINLENGTH * x->m_sr) * MAX_PITCH) * sizeof(double));
		if (x->grainmem[i].right == NULL) {
			object_error((t_object *)x, "out of memory");
			return NULL;
		}
	}

	/************************************************************************************************************************/
	// INITIALIZE VALUES
	x->object_inlets[0] = 0.0; // initialize float inlet value for current start min value
	x->object_inlets[1] = 0.0; // initialize float inlet value for current start max value
	x->object_inlets[2] = 150; // initialize float inlet value for min grain length
	x->object_inlets[3] = 150; // initialize float inlet value for max grain length
	x->object_inlets[4] = 1.0; // initialize inlet value for min pitch
	x->object_inlets[5] = 1.0; // initialize inlet value for min pitch
	x->object_inlets[6] = 0.0; // initialize value for min pan
	x->object_inlets[7] = 0.0; // initialize value for max pan
	x->object_inlets[8] = 1.0; // initialize value for min gain
	x->object_inlets[9] = 1.0; // initialize value for max gain
	x->tr_prev = 0.0; // initialize value for previous trigger sample
	x->grains_count = 0; // initialize the grains count value
	x->grains_limit_old = 0; // initialize value for the routine when grains limit was modified
	x->limit_modified = 0; // initialize channel change flag
	x->buffer_modified = 0; // initialize buffer modified flag
	x->w_writeflag = 0; // initialize window write flag

	// initialize the testvalues which are not dependent on sampleRate
	x->testvalues[0] = 0.0; // dummy MIN_START
	x->testvalues[1] = 0.0; // dummy MAX_START
	x->testvalues[4] = MIN_PITCH;
	x->testvalues[5] = MAX_PITCH;
	x->testvalues[6] = MIN_PAN;
	x->testvalues[7] = MAX_PAN;
	x->testvalues[8] = MIN_GAIN;
	x->testvalues[9] = MAX_GAIN;

	// calculate constants for panning function
	x->piovr2 = 4.0 * atan(1.0) * 0.5;
	x->root2ovr2 = sqrt(2.0) * 0.5;

	// bang trigger flag
	x->bang_trigger = 0;
	
	// grainmem structure members
	for (i = 0; i < MAXGRAINS; i++) {
		x->grainmem[i].length = 0;
		x->grainmem[i].pos = 0;
		x->grainmem[i].busy = 0;
	}

	/************************************************************************************************************************/
	// BUFFER REFERENCES
	x->buffer = buffer_ref_new((t_object *)x, x->buffer_name); // write the buffer reference into the object structure


	// WRITE WINDOW INTO WINDOW ARRAY
	cmindexcloud_windowwrite(x);

	#ifdef WIN_VERSION
		srand((unsigned int)clock());
	#endif

	return x;
}


/************************************************************************************************************************/
/* THE 64 BIT DSP METHOD                                                                                                */
/************************************************************************************************************************/
void cmindexcloud_dsp64(t_cmindexcloud *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags) {
	int i;
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
		for (i = 0; i < MAXGRAINS; i++) {
			x->grainmem[i].left = (double *)sysmem_resizeptrclear(x->grainmem[i].left, ((MAX_GRAINLENGTH * x->m_sr) * MAX_PITCH) * sizeof(double));
			if (x->grainmem[i].left == NULL) {
				object_error((t_object *)x, "out of memory");
				return;
			}
		}
		for (i = 0; i < MAXGRAINS; i++) {
			x->grainmem[i].right = (double *)sysmem_resizeptrclear(x->grainmem[i].right, ((MAX_GRAINLENGTH * x->m_sr) * MAX_PITCH) * sizeof(double));
			if (x->grainmem[i].right == NULL) {
				object_error((t_object *)x, "out of memory");
				return;
			}
		}
	}
	// calcuate the sampleRate-dependant test values
	x->testvalues[2] = MIN_GRAINLENGTH * x->m_sr;
	x->testvalues[3] = MAX_GRAINLENGTH * x->m_sr;

	// CALL THE PERFORM ROUTINE
	object_method(dsp64, gensym("dsp_add64"), x, cmindexcloud_perform64, 0, NULL);
	//	dsp_add64(dsp64, (t_object*)x, (t_perfroutine64)cmindexcloud_perform64, 0, NULL);
}


/************************************************************************************************************************/
/* THE 64 BIT PERFORM ROUTINE                                                                                           */
/************************************************************************************************************************/
void cmindexcloud_perform64(t_cmindexcloud *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam) {
	// VARIABLE DECLARATIONS
	short trigger = 0; // trigger occurred yes/no
	long i, limit, r; // for loop counterS
	long n = sampleframes; // number of samples per signal vector
	double tr_curr; // current trigger value
	double distance; // floating point index for reading from buffers
	long index; // truncated index for reading from buffers
	double b_read, w_read; // current sample read from the sample buffer and window array
	double outsample_left = 0.0; // temporary left output sample used for adding up all grain samples
	double outsample_right = 0.0; // temporary right output sample used for adding up all grain samples
	int slot = 0; // variable for the current slot in the arrays to write grain info to
	cm_panstruct panstruct; // struct for holding the calculated constant power left and right stereo values

	long readpos;
	long start;
	long smp_length;
	long pitch_length;
	double gain;
	double pan_left, pan_right;
	
	// OUTLETS
	t_double *out_left 	= (t_double *)outs[0]; // assign pointer to left output
	t_double *out_right = (t_double *)outs[1]; // assign pointer to right output

	// BUFFER VARIABLE DECLARATIONS
	t_buffer_obj *buffer = buffer_ref_getobject(x->buffer);
	float *b_sample = buffer_locksamples(buffer);
	long b_framecount; // number of frames in the sample buffer
	t_atom_long b_channelcount; // number of channels in the sample buffer

	//float *w_sample = (float *)x->window;


	// BUFFER CHECKS
	if (!b_sample) { // if the sample buffer does not exist
		goto zero;
	}

	// GET BUFFER INFORMATION
	b_framecount = buffer_getframecount(buffer); // get number of frames in the sample buffer
	b_channelcount = buffer_getchannelcount(buffer); // get number of channels in the sample buffer

	// GET INLET VALUES
	t_double *tr_sigin 	= (t_double *)ins[0]; // get trigger input signal from 1st inlet

	x->grain_params[0] = x->connect_status[0] ? *ins[1] * x->m_sr : x->object_inlets[0] * x->m_sr;	// start min
	x->grain_params[1] = x->connect_status[1] ? *ins[2] * x->m_sr : x->object_inlets[1] * x->m_sr;	// start max
	x->grain_params[2] = x->connect_status[2] ? *ins[3] * x->m_sr : x->object_inlets[2] * x->m_sr;	// length min
	x->grain_params[3] = x->connect_status[3] ? *ins[4] * x->m_sr : x->object_inlets[3] * x->m_sr;	// length max
	x->grain_params[4] = x->connect_status[4] ? *ins[5] : x->object_inlets[4];						// pitch min
	x->grain_params[5] = x->connect_status[5] ? *ins[6] : x->object_inlets[5];						// pitch max
	x->grain_params[6] = x->connect_status[6] ? *ins[7] : x->object_inlets[6];						// pan min
	x->grain_params[7] = x->connect_status[7] ? *ins[8] : x->object_inlets[7];						// pan max
	x->grain_params[8] = x->connect_status[8] ? *ins[9] : x->object_inlets[8];						// gain min
	x->grain_params[9] = x->connect_status[9] ? *ins[10] : x->object_inlets[9];						// gain max


	// DSP LOOP
	while (n--) {
		tr_curr = *tr_sigin++; // get current trigger value

		if (x->attr_zero) {
			if (signbit(tr_curr) != signbit(x->tr_prev)) { // zero crossing from negative to positive
				trigger = 1;
			}
			else if (x->bang_trigger) {
				trigger = 1;
				x->bang_trigger = 0;
			}
		}
		else {
			if ((x->tr_prev - tr_curr) > 0.9) {
				trigger = 1;
			}
			else if (x->bang_trigger) {
				trigger = 1;
				x->bang_trigger = 0;
			}
		}
		
		/************************************************************************************************************************/
		// IN CASE OF TRIGGER, LIMIT NOT MODIFIED AND GRAINS COUNT IN THE LEGAL RANGE (AVAILABLE SLOTS)
		if (trigger && x->grains_count < x->grains_limit && !x->limit_modified && !x->w_writeflag && !x->buffer_modified && !x->w_window_modified) {
			trigger = 0; // reset trigger
			x->grains_count++; // increment grains_count
			// FIND A FREE SLOT FOR THE NEW GRAIN
			i = 0;
			while (i < x->grains_limit) {
				if (!x->grainmem[i].busy) {
					x->grainmem[i].busy = 1;
					slot = i;
					break;
				}
				i++;
			}

			// randomize the grain parameters and write them into the randomized array
			x->randomized[0] = cm_random(&x->grain_params[0], &x->grain_params[1]); // start
			x->randomized[1] = cm_random(&x->grain_params[2], &x->grain_params[3]); // length
			x->randomized[2] = cm_random(&x->grain_params[4], &x->grain_params[5]); // pitch
			x->randomized[3] = cm_random(&x->grain_params[6], &x->grain_params[7]); // pan
			x->randomized[4] = cm_random(&x->grain_params[8], &x->grain_params[9]); // gain

			// check for parameter sanity of the length value
			if (x->randomized[1] < x->testvalues[2]) {
				x->randomized[1] = x->testvalues[2];
			}
			else if (x->randomized[1] > x->testvalues[3]) {
				x->randomized[1] = x->testvalues[3];
			}

			// check for parameter sanity of the pitch value
			if (x->randomized[2] < x->testvalues[4]) {
				x->randomized[2] = x->testvalues[4];
			}
			else if (x->randomized[2] > x->testvalues[5]) {
				x->randomized[2] = x->testvalues[5];
			}

			// check for parameter sanity of the pan value
			if (x->randomized[3] < x->testvalues[6]) {
				x->randomized[3] = x->testvalues[6];
			}
			else if (x->randomized[3] > x->testvalues[7]) {
				x->randomized[3] = x->testvalues[7];
			}

			// check for parameter sanity of the gain value
			if (x->randomized[4] < x->testvalues[8]) {
				x->randomized[4] = x->testvalues[8];
			}
			else if (x->randomized[4] > x->testvalues[9]) {
				x->randomized[4] = x->testvalues[9];
			}

			// write grain lenght slot (non-pitch)
			smp_length = x->randomized[1];
			pitch_length = smp_length * x->randomized[2]; // length * pitch
			// check that grain length is not larger than size of buffer
			if (pitch_length > b_framecount) {
				pitch_length = b_framecount;
			}
			x->grainmem[slot].length = smp_length; // IMPORTANT!! DO NOT FORGET TO WRITE THE SAMPLE LENGTH INTO THE MEMORY STRUCTURE
			
			// write start position
			start = x->randomized[0];
			// start position sanity testing
			if (start > b_framecount - (pitch_length + 1)) { // plus 1 because of interpolation sample increment
				start = b_framecount - (pitch_length + 1);
			}
			if (start < 0) {
				start = 0;
			}
			// compute pan values
			cm_panning(&panstruct, &x->randomized[3], x); // calculate pan values in panstruct
			pan_left = panstruct.left;
			pan_right = panstruct.right;
			// write gain value
			gain = x->randomized[4];
			
			
			// grain is written into memory here
			for (readpos = 0; readpos < smp_length; readpos++) {
				if (x->attr_winterp) {
					distance = ((double)readpos / (double)smp_length) * (double)x->window_length;
					w_read = cm_lininterpwin(distance, x->window, 1, 0);
				}
				else {
					index = (long)(((double)readpos / (double)smp_length) * (double)x->window_length);
					w_read = x->window[index];
				}
				// GET GRAIN SAMPLE FROM SAMPLE BUFFER
				distance = start + (((double)readpos / (double)smp_length) * (double)pitch_length);
				
				if (b_channelcount > 1 && x->attr_stereo) { // if more than one channel
					if (x->attr_sinterp) {
						// get interpolated sample
						x->grainmem[slot].left[readpos] = ((cm_lininterp(distance, b_sample, b_channelcount, 0) * w_read) * pan_left) * gain;
						x->grainmem[slot].right[readpos] = ((cm_lininterp(distance, b_sample, b_channelcount, 1) * w_read) * pan_right) * gain;
					}
					else {
						// get non-interpolated sample
						x->grainmem[slot].left[readpos] = ((b_sample[(long)distance * b_channelcount] * w_read) * pan_left) * gain;
						x->grainmem[slot].right[readpos] = ((b_sample[((long)distance * b_channelcount) + 1] * w_read) * pan_right) * gain;
					}
				}
				else { // if only one channel
					if (x->attr_sinterp) {
						b_read = cm_lininterp(distance, b_sample, b_channelcount, 0) * w_read; // get interpolated sample
						x->grainmem[slot].left[readpos] = (b_read * pan_left) * gain;
						x->grainmem[slot].right[readpos] = (b_read * pan_right) * gain;
					}
					else {
						x->grainmem[slot].left[readpos] = ((b_sample[(long)distance * b_channelcount] * w_read) * pan_left) * gain;
						x->grainmem[slot].right[readpos] = ((b_sample[(long)distance * b_channelcount] * w_read) * pan_right) * gain;
					}
				}
			}
		}
		/************************************************************************************************************************/
		// CONTINUE WITH THE PLAYBACK ROUTINE
		if (x->limit_modified) {
			limit = x->grains_limit_old;
		}
		else {
			limit = x->grains_limit;
		}
		
		for (i = 0; i < limit; i++) {
			if (x->grainmem[i].busy) {
				r = x->grainmem[i].pos++;
				outsample_left += x->grainmem[i].left[r];
				outsample_right += x->grainmem[i].right[r];
				if (x->grainmem[i].pos == x->grainmem[i].length) {
					x->grainmem[i].pos = 0;
					x->grainmem[i].busy = 0;
					x->grains_count--;
					if (x->grains_count < 0) {
						x->grains_count = 0;
					}
				}
			}
		}
		
		// CHECK IF GRAINS COUNT IS ZERO, THEN RESET LIMIT_MODIFIED CHECKFLAG
		if (x->grains_count == 0) {
			x->limit_modified = 0; // reset limit modified checkflag
			x->buffer_modified = 0;
			x->w_window_modified = 0;
		}

		/************************************************************************************************************************/
		x->tr_prev = tr_curr; // store current trigger value in object structure
		
		*out_left++ = outsample_left; // write added sample values to left output vector
		*out_right++ = outsample_right; // write added sample values to right output vector
		
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
void cmindexcloud_assist(t_cmindexcloud *x, void *b, long msg, long arg, char *dst) {
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
void cmindexcloud_free(t_cmindexcloud *x) {
	int i;
	dsp_free((t_pxobject *)x); // free memory allocated for the object
	object_free(x->buffer); // free the buffer reference

	sysmem_freeptr(x->window); // free memory allocated to the window array

	for (i = 0; i < MAXGRAINS; i++) {
		sysmem_freeptr(x->grainmem[i].left);
		sysmem_freeptr(x->grainmem[i].right);
	}
	sysmem_freeptr(x->grainmem);
	
	sysmem_freeptr(x->object_inlets); // free memory allocated to the object inlets array
	sysmem_freeptr(x->grain_params); // free memory allocated to the grain parameters array
	sysmem_freeptr(x->randomized); // free memory allocated to the grain parameters array
	sysmem_freeptr(x->testvalues); // free memory allocated to the test values array
}

/************************************************************************************************************************/
/* FLOAT METHOD FOR FLOAT INLET SUPPORT                                                                                 */
/************************************************************************************************************************/
void cmindexcloud_float(t_cmindexcloud *x, double f) {
	double dump;
	int inlet = ((t_pxobject*)x)->z_in; // get info as to which inlet was addressed (stored in the z_in component of the object structure
	switch (inlet) {
		case 1: // first inlet
			if (f < 0.0) {
				dump = f;
			}
			else {
				x->object_inlets[0] = f;
			}
			break;
		case 2: // second inlet
			if (f < 0.0) {
				dump = f;
			}
			else {
				x->object_inlets[1] = f;
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
				x->object_inlets[2] = f;
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
				x->object_inlets[3] = f;
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
				x->object_inlets[4] = f;
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
				x->object_inlets[5] = f;
			}
			break;
		case 7:
			if (f < -1.0 || f > 1.0) {
				dump = f;
			}
			else {
				x->object_inlets[6] = f;
			}
			break;
		case 8:
			if (f < -1.0 || f > 1.0) {
				dump = f;
			}
			else {
				x->object_inlets[7] = f;
			}
			break;
		case 9:
			if (f < 0.0 || f > MAX_GAIN) {
				dump = f;
			}
			else {
				x->object_inlets[8] = f;
			}
			break;
		case 10:
			if (f < 0.0 || f > MAX_GAIN) {
				dump = f;
			}
			else {
				x->object_inlets[9] = f;
			}
			break;
	}
}


/************************************************************************************************************************/
/* DOUBLE CLICK METHOD FOR VIEWING BUFFER CONTENT                                                                       */
/************************************************************************************************************************/
void cmindexcloud_dblclick(t_cmindexcloud *x) {
	buffer_view(buffer_ref_getobject(x->buffer));
}


/************************************************************************************************************************/
/* NOTIFY METHOD FOR THE BUFFER REFERENCES                                                                              */
/************************************************************************************************************************/
t_max_err cmindexcloud_notify(t_cmindexcloud *x, t_symbol *s, t_symbol *msg, void *sender, void *data) {
	if (msg == ps_buffer_modified) {
		x->buffer_modified = 1;
	}
	return buffer_ref_notify(x->buffer, s, msg, sender, data); // return with the calling buffer
}


/************************************************************************************************************************/
/* THE ACTUAL BUFFER SET METHOD                                                                                         */
/************************************************************************************************************************/
void cmindexcloud_doset(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av) {
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
/* THE BUFFER SET METHOD                                                                                                */
/************************************************************************************************************************/
void cmindexcloud_set(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av) {
	defer(x, (method)cmindexcloud_doset, s, ac, av);
}


/************************************************************************************************************************/
/* THE ACTUAL WINDOW TYPE SET METHOD                                                                                    */
/************************************************************************************************************************/
void cmindexcloud_do_w_type(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av) {
	long arg = atom_getlong(av);
	if (ac && av) {
		if (!x->w_writeflag) { // only if the window array is not currently being rewritten
			// CHECK IF WINDOW TYPE ARGUMENT IS VALID
			if (arg < 0 || arg > MAX_WININDEX) {
				object_error((t_object *)x, "invalid window type");
			}
			else {
				x->w_writeflag = 1;
				x->w_window_modified = 1;
				x->window_type = arg; // write window type into object structure
				cmindexcloud_windowwrite(x); // write window into window array
				x->w_writeflag = 0;
			}
		}
	}
	else {
		object_error((t_object *)x, "argument required (window type)");
	}
}


/************************************************************************************************************************/
/* THE WINDOW TYPE SET METHOD                                                                                           */
/************************************************************************************************************************/
void cmindexcloud_w_type(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av) {
	defer(x, (method)cmindexcloud_do_w_type, s, ac, av);
}



/************************************************************************************************************************/
/* THE ACTUAL WINDOW LENGTH SET METHOD                                                                                  */
/************************************************************************************************************************/
void cmindexcloud_do_w_length(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av) {
	long arg = atom_getlong(av);
	if (ac && av) {
		// CHECK IF WINDOW LENGTH ARGUMENT IS VALID
		if (arg < MIN_WINDOWLENGTH) {
			object_error((t_object *)x, "window length must be greater than %d", MIN_WINDOWLENGTH);
		}
		else if (!x->w_writeflag) { // only if the window array is not currently being rewritten
			x->w_writeflag = 1;
			x->w_window_modified = 1;
			x->window_length = arg; // write window length into object structure
			x->window = (double *)sysmem_resizeptrclear(x->window, x->window_length * sizeof(double)); // resize and clear window array
			cmindexcloud_windowwrite(x); // write window into window array
			x->w_writeflag = 0;
		}
	}
	else {
		object_error((t_object *)x, "argument required (window length)");
	}
}


/************************************************************************************************************************/
/* THE WINDOW LENGTH SET METHOD                                                                                         */
/************************************************************************************************************************/
void cmindexcloud_w_length(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av) {
	defer(x, (method)cmindexcloud_do_w_length, s, ac, av);
}




/************************************************************************************************************************/
/* THE GRAINS LIMIT METHOD                                                                                              */
/************************************************************************************************************************/
void cmindexcloud_limit(t_cmindexcloud *x, t_symbol *s, long ac, t_atom *av) {
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
/* THE BANG METHOD                                                                                                      */
/************************************************************************************************************************/
void cmindexcloud_bang(t_cmindexcloud *x) {
	x->bang_trigger = 1;
}


/************************************************************************************************************************/
/* THE STEREO ATTRIBUTE SET METHOD                                                                                      */
/************************************************************************************************************************/
t_max_err cmindexcloud_stereo_set(t_cmindexcloud *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_stereo = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}


/************************************************************************************************************************/
/* THE WINDOW INTERPOLATION ATTRIBUTE SET METHOD                                                                        */
/************************************************************************************************************************/
t_max_err cmindexcloud_winterp_set(t_cmindexcloud *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_winterp = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}


/************************************************************************************************************************/
/* THE SAMPLE INTERPOLATION ATTRIBUTE SET METHOD                                                                        */
/************************************************************************************************************************/
t_max_err cmindexcloud_sinterp_set(t_cmindexcloud *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_sinterp = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}


/************************************************************************************************************************/
/* THE ZERO CROSSING ATTRIBUTE SET METHOD                                                                               */
/************************************************************************************************************************/
t_max_err cmindexcloud_zero_set(t_cmindexcloud *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_zero = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}

/************************************************************************************************************************/
/* THE WINDOW_WRITE FUNCTION                                                                                            */
/************************************************************************************************************************/
void cmindexcloud_windowwrite(t_cmindexcloud *x) {
	//	int i;
	long length = x->window_length;
	switch (x->window_type) {
		case 0:
			object_post((t_object*)x, "hann - %d", length);
			cm_hann(x->window, &length);
			break;
		case 1:
			object_post((t_object*)x, "hamming - %d", length);
			cm_hamming(x->window, &length);
			break;
		case 2:
			object_post((t_object*)x, "rectangular - %d", length);
			cm_rectangular(x->window, &length);
			break;
		case 3:
			object_post((t_object*)x, "bartlett - %d", length);
			cm_bartlett(x->window, &length);
			break;
		case 4:
			object_post((t_object*)x, "flattop - %d", length);
			cm_flattop(x->window, &length);
			break;
		case 5:
			object_post((t_object*)x, "gauss (alpha 2) - %d", length);
			cm_gauss2(x->window, &length);
			break;
		case 6:
			object_post((t_object*)x, "gauss (alpha 4) - %d", length);
			cm_gauss4(x->window, &length);
			break;
		case 7:
			object_post((t_object*)x, "gauss (alpha 8) - %d", length);
			cm_gauss8(x->window, &length);
			break;
		default:
			cm_hann(x->window, &length);
	}
	return;
}


/************************************************************************************************************************/
/* CUSTOM FUNCTIONS																										*/
/************************************************************************************************************************/
// constant power stereo function
void cm_panning(cm_panstruct *panstruct, double *pos, t_cmindexcloud *x) {
	panstruct->left = x->root2ovr2 * (cos((*pos * x->piovr2) * 0.5) - sin((*pos * x->piovr2) * 0.5));
	panstruct->right = x->root2ovr2 * (cos((*pos * x->piovr2) * 0.5) + sin((*pos * x->piovr2) * 0.5));
	return;
}
// RANDOM NUMBER GENERATOR (USE POINTERS FOR MORE EFFICIENCY)
double cm_random(double *min, double *max) {
#ifdef MAC_VERSION
	return *min + ((*max - *min) * (((double)arc4random_uniform(RANDMAX)) / (double)RANDMAX));
#endif
#ifdef WIN_VERSION
	return *min + ((*max - *min) * ((double)(rand() % RANDMAX) / (double)RANDMAX));
#endif
}
// LINEAR INTERPOLATION FUNCTION
double cm_lininterp(double distance, float *buffer, t_atom_long b_channelcount, short channel) {
	long index = (long)distance; // get truncated index
	distance -= (long)distance; // calculate fraction value for interpolation
	return buffer[index * b_channelcount + channel] + distance * (buffer[(index + 1) * b_channelcount + channel] - buffer[index * b_channelcount + channel]);
}
// LINEAR INTERPOLATION FUNCTION FOR WINDOW (passing douple pointer)
double cm_lininterpwin(double distance, double *buffer, t_atom_long b_channelcount, short channel) {
	long index = (long)distance; // get truncated index
	distance -= (long)distance; // calculate fraction value for interpolation
	return buffer[index * b_channelcount + channel] + distance * (buffer[(index + 1) * b_channelcount + channel] - buffer[index * b_channelcount + channel]);
}


/************************************************************************************************************************/
/* WINDOW FUNCTIONS																										*/
/************************************************************************************************************************/
void cm_hann(double *window, long *length) {
	int i;
	long N = *length - 1;
	for (i = 0; i < *length; i++) {
		window[i] = 0.5 * (1.0 - cos((2.0 * M_PI) * ((double)i / (double)N)));
	}
	return;
}


void cm_hamming(double *window, long *length) {
	int i;
	long N = *length - 1;
	for (i = 0; i < *length; i++) {
		window[i] = 0.54 - (0.46 * cos((2.0 * M_PI) * ((double)i / (double)N)));
	}
	return;
}


void cm_rectangular(double *window, long *length) {
	int i;
	for (i = 0; i < *length; i++) {
		window[i] = 1;
	}
	return;
}


void cm_bartlett(double *window, long *length) {
	int i;
	long N = *length - 1;
	for (i = 0; i < *length; i++) {
		if (i < (*length / 2)) {
			window[i] = (2 * (double)i) / (double)N;
		}
		else {
			window[i] = 2 - ((2 * (double)i) / (double)N);
		}
	}
	return;
}


void cm_flattop(double *window, long *length) {
	int i;
	long N = *length - 1;
	double a0 = 0.21557895;
	double a1 = 0.41663158;
	double a2 = 0.277263158;
	double a3 = 0.083578947;
	double a4 = 0.006947368;
	double twopi = M_PI * 2.0;
	double fourpi = M_PI * 4.0;
	double sixpi = M_PI * 6.0;
	double eightpi = M_PI * 8.0;
	for (i = 0; i < *length; i++) {
		window[i] = a0 - (a1 * cos((twopi * i) / N)) + (a2 * cos((fourpi * i) / N)) - (a3 * cos((sixpi * i) / N)) + (a4 * cos((eightpi * i) / N));
	}
	return;
}


void cm_gauss2(double *window, long *length) {
	int i;
	double n;
	double N = *length - 1;
	double alpha = 2.0;
	double stdev = N / (2 * alpha);
	for (i = 0; i < *length; i++) {
		n = i - N / 2;
		window[i] = exp(-0.5 * pow((n / stdev), 2));
	}
}


void cm_gauss4(double *window, long *length) {
	int i;
	double n;
	double N = *length - 1;
	double alpha = 4.0;
	double stdev = N / (2 * alpha);
	for (i = 0; i < *length; i++) {
		n = i - N / 2;
		window[i] = exp(-0.5 * pow((n / stdev), 2));
	}
}


void cm_gauss8(double *window, long *length) {
	int i;
	double n;
	double N = *length - 1;
	double alpha = 8.0;
	double stdev = N / (2 * alpha);
	for (i = 0; i < *length; i++) {
		n = i - N / 2;
		window[i] = exp(-0.5 * pow((n / stdev), 2));
	}
}
