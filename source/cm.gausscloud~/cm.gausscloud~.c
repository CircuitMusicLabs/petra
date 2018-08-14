/*
 cm.gausscloud~ - a granular synthesis external audio object for Max/MSP.
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
#define MIN_CLOUDSIZE 1 // min cloud size in ms
#define MIN_GRAINLENGTH 1 // min grain length in ms
#define MIN_PITCH 0.001 // min pitch
#define MAX_PITCH 8 // max pitch
#define MIN_PAN -1.0 // min pan
#define MAX_PAN 1.0 // max pan
#define MIN_GAIN 0.0 // min gain
#define MAX_GAIN 2.0  // max gain
#define MIN_ALPHA 0.1 // min alpha value
#define MAX_ALPHA 10.0 // max alpha value
#define ARGUMENTS 3 // constant number of arguments required for the external
#define FLOAT_INLETS 12 // number of object float inlets
#define PITCHLIST 10 // max values to be provided for pitch list
#define RANDMAX 10000


/************************************************************************************************************************/
/* GRAIN MEMORY STORAGE                                                                                                 */
/************************************************************************************************************************/
typedef struct cmcloud {
	double *left;
	double *right;
	long length;
	long pos;
	t_bool busy; // used to store the flag if a grain is currently playing or not
} cm_cloud;


/************************************************************************************************************************/
/* OBJECT STRUCTURE                                                                                                     */
/************************************************************************************************************************/
typedef struct _cmgausscloud {
	t_pxobject obj;
	t_symbol *buffer_name; // sample buffer name
	t_buffer_ref *buffer; // sample buffer reference
	double m_sr; // system millisampling rate (samples per milliseconds = sr * 0.001)
	short connect_status[FLOAT_INLETS]; // array for signal inlet connection statuses
	double *object_inlets; // array to store the incoming values coming from the object inlets
	double *grain_params; // array to store the processed values coming from the object inlets
	double *randomized; // array to store the randomized grain values
	double tr_prev; // trigger sample from previous signal vector (required to check if input ramp resets to zero)
	t_bool buffer_modified; // checkflag to see if buffer has been modified
	short grains_count; // currently playing grains
	void *grains_count_out; // outlet for number of currently playing grains (for debugging)
	t_atom_long attr_stereo; // attribute: number of channels to be played
	t_atom_long attr_sinterp; // attribute: window interpolation on/off
	t_atom_long attr_zero; // attribute: zero crossing trigger on/off
	double piovr2; // pi over two for panning function
	double root2ovr2; // root of 2 over two for panning function
	t_bool bang_trigger;
	cm_cloud *cloud; // struct array for storing the grains and associated variables in memory
	long cloudsize; // size of the cloud struct array, value obtained from argument and "cloudsize" method
	t_bool resize_request; // flag set to true when "cloudsize" method called
	long cloudsize_new; // new cloudsize obtained from "cloudsize" method
	t_bool resize_verify; // check-flag for proper memroy re-allocation
	long grainlength; // maximum grain length
	t_bool length_request; // flag set to true when "grainlength" method called
	long grainlength_new; // new grain length obtained from "grainlength" method
	t_bool length_verify; // check flag for proper memory re-allocation
	double *pitchlist; // array to store pitch values provided by method
	double pitchlist_zero; // zero value pointer for randomize function
	double pitchlist_size; // current numer of values stored in the pitch list array
	t_bool pitchlist_active; // boolean pitch list active true/false
} t_cmgausscloud;


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
static t_class *cmgausscloud_class; // class pointer
static t_symbol *ps_buffer_modified, *ps_stereo;


/************************************************************************************************************************/
/* FUNCTION PROTOTYPES                                                                                                  */
/************************************************************************************************************************/
void *cmgausscloud_new(t_symbol *s, long argc, t_atom *argv);
void cmgausscloud_dsp64(t_cmgausscloud *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void cmgausscloud_perform64(t_cmgausscloud *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void cmgausscloud_assist(t_cmgausscloud *x, void *b, long msg, long arg, char *dst);
void cmgausscloud_free(t_cmgausscloud *x);
void cmgausscloud_float(t_cmgausscloud *x, double f);
void cmgausscloud_dblclick(t_cmgausscloud *x);
t_max_err cmgausscloud_notify(t_cmgausscloud *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
void cmgausscloud_set(t_cmgausscloud *x, t_symbol *s, long ac, t_atom *av);
void cmgausscloud_cloudsize(t_cmgausscloud *x, t_symbol *s, long ac, t_atom *av);
void cmgausscloud_grainlength(t_cmgausscloud *x, t_symbol *s, long ac, t_atom *av);
void cmgausscloud_pitchlist(t_cmgausscloud *x, t_symbol *s, long ac, t_atom *av);
void cmgausscloud_bang(t_cmgausscloud *x);
t_bool cmgausscloud_resize(t_cmgausscloud *x);

t_max_err cmgausscloud_stereo_set(t_cmgausscloud *x, t_object *attr, long argc, t_atom *argv);
t_max_err cmgausscloud_sinterp_set(t_cmgausscloud *x, t_object *attr, long argc, t_atom *argv);
t_max_err cmgausscloud_zero_set(t_cmgausscloud *x, t_object *attr, long argc, t_atom *argv);

// PANNING FUNCTION
void cm_panning(cm_panstruct *panstruct, double *pos, t_cmgausscloud *x);
// RANDOM NUMBER GENERATOR
double cm_random(double *min, double *max);
// LINEAR INTERPOLATION FUNCTION
double cm_lininterp(double distance, float *b_sample, t_atom_long b_channelcount, t_atom_long b_framecount, short channel);
// GAUSS WINDOW FUNCTION
double cm_gauss(long *pos, long *length, double *alpha);


/************************************************************************************************************************/
/* MAIN FUNCTION (INITIALIZATION ROUTINE)                                                                               */
/************************************************************************************************************************/
void ext_main(void *r) {
	// Initialize the class - first argument: VERY important to match the name of the object in the procect settings!!!
	cmgausscloud_class = class_new("cm.gausscloud~", (method)cmgausscloud_new, (method)cmgausscloud_free, sizeof(t_cmgausscloud), 0, A_GIMME, 0);

	class_addmethod(cmgausscloud_class, (method)cmgausscloud_dsp64, 		"dsp64", 		A_CANT, 0);  // Bind the 64 bit dsp method
	class_addmethod(cmgausscloud_class, (method)cmgausscloud_assist, 		"assist", 		A_CANT, 0); // Bind the assist message
	class_addmethod(cmgausscloud_class, (method)cmgausscloud_float, 		"float", 		A_FLOAT, 0); // Bind the float message (allowing float input)
	class_addmethod(cmgausscloud_class, (method)cmgausscloud_dblclick,		"dblclick",		A_CANT, 0); // Bind the double click message
	class_addmethod(cmgausscloud_class, (method)cmgausscloud_notify, 		"notify", 		A_CANT, 0); // Bind the notify message
	class_addmethod(cmgausscloud_class, (method)cmgausscloud_set,			"set", 			A_GIMME, 0); // Bind the set message for user buffer set
	class_addmethod(cmgausscloud_class, (method)cmgausscloud_cloudsize,		"cloudsize",	A_GIMME, 0); // Bind the cloudsize message
	class_addmethod(cmgausscloud_class, (method)cmgausscloud_grainlength,	"grainlength",	A_GIMME, 0); // Bind the grainlength message
	class_addmethod(cmgausscloud_class, (method)cmgausscloud_pitchlist,		"pitchlist",	A_GIMME, 0); // Bind the pitchlist message
	class_addmethod(cmgausscloud_class, (method)cmgausscloud_bang,			"bang",			0);

	CLASS_ATTR_ATOM_LONG(cmgausscloud_class, "stereo", 0, t_cmgausscloud, attr_stereo);
	CLASS_ATTR_ACCESSORS(cmgausscloud_class, "stereo", (method)NULL, (method)cmgausscloud_stereo_set);
	CLASS_ATTR_BASIC(cmgausscloud_class, "stereo", 0);
	CLASS_ATTR_SAVE(cmgausscloud_class, "stereo", 0);
	CLASS_ATTR_STYLE_LABEL(cmgausscloud_class, "stereo", 0, "onoff", "Multichannel playback");

	CLASS_ATTR_ATOM_LONG(cmgausscloud_class, "s_interp", 0, t_cmgausscloud, attr_sinterp);
	CLASS_ATTR_ACCESSORS(cmgausscloud_class, "s_interp", (method)NULL, (method)cmgausscloud_sinterp_set);
	CLASS_ATTR_BASIC(cmgausscloud_class, "s_interp", 0);
	CLASS_ATTR_SAVE(cmgausscloud_class, "s_interp", 0);
	CLASS_ATTR_STYLE_LABEL(cmgausscloud_class, "s_interp", 0, "onoff", "Sample interpolation on/off");

	CLASS_ATTR_ATOM_LONG(cmgausscloud_class, "zero", 0, t_cmgausscloud, attr_zero);
	CLASS_ATTR_ACCESSORS(cmgausscloud_class, "zero", (method)NULL, (method)cmgausscloud_zero_set);
	CLASS_ATTR_BASIC(cmgausscloud_class, "zero", 0);
	CLASS_ATTR_SAVE(cmgausscloud_class, "zero", 0);
	CLASS_ATTR_STYLE_LABEL(cmgausscloud_class, "zero", 0, "onoff", "Zero crossing trigger mode on/off");

	CLASS_ATTR_ORDER(cmgausscloud_class, "stereo", 0, "1");
	CLASS_ATTR_ORDER(cmgausscloud_class, "s_interp", 0, "2");
	CLASS_ATTR_ORDER(cmgausscloud_class, "zero", 0, "3");

	class_dspinit(cmgausscloud_class); // Add standard Max/MSP methods to your class
	class_register(CLASS_BOX, cmgausscloud_class); // Register the class with Max
	ps_buffer_modified = gensym("buffer_modified"); // assign the buffer modified message to the static pointer created above
	ps_stereo = gensym("stereo");

}


/************************************************************************************************************************/
/* NEW INSTANCE ROUTINE                                                                                                 */
/************************************************************************************************************************/
void *cmgausscloud_new(t_symbol *s, long argc, t_atom *argv) {
	long i;
	t_cmgausscloud *x = (t_cmgausscloud *)object_alloc(cmgausscloud_class); // create the object and allocate required memory
	dsp_setup((t_pxobject *)x, 13); // create 13 inlets

	if (argc < ARGUMENTS) {
		object_error((t_object *)x, "%d arguments required: sample buffer | cloud size | max. grain length", ARGUMENTS);
		return NULL;
	}

	x->buffer_name = atom_getsymarg(0, argc, argv); // get user supplied argument for sample buffer
	x->cloudsize = atom_getintarg(1, argc, argv); // get user supplied argument for cloud size
	x->grainlength = atom_getintarg(2, argc, argv); // get user supplied argument for maximum grain length


	// HANDLE ATTRIBUTES
	object_attr_setlong(x, gensym("stereo"), 0); // initialize stereo attribute
	object_attr_setlong(x, gensym("s_interp"), 1); // initialize window interpolation attribute
	object_attr_setlong(x, gensym("zero"), 0); // initialize zero crossing attribute
	attr_args_process(x, argc, argv); // get attribute values if supplied as argument

	// CHECK IF USER SUPPLIED MAXIMUM GRAINS IS IN THE LEGAL RANGE
	if (x->cloudsize < MIN_CLOUDSIZE) {
		object_error((t_object *)x, "cloud size must be equal to or larger than %d", MIN_CLOUDSIZE);
		return NULL;
	}
	
	// CHECK IF USER SUPPLIED MAXIMUM GRAINS IS IN THE LEGAL RANGE
	if (x->grainlength < MIN_GRAINLENGTH) {
		object_error((t_object *)x, "maximum grain length must be equal to or larger than %d", MIN_GRAINLENGTH);
		return NULL;
	}

	// CREATE OUTLETS (OUTLETS ARE CREATED FROM RIGHT TO LEFT)
	x->grains_count_out = intout((t_object *)x); // create outlet for number of currently playing grains
	outlet_new((t_object *)x, "signal"); // right signal outlet
	outlet_new((t_object *)x, "signal"); // left signal outlet

	// GET SYSTEM SAMPLE RATE
	x->m_sr = sys_getsr() * 0.001; // get the current sample rate and write it into the object structure
	

	/************************************************************************************************************************/
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

	// ALLOCATE MEMORY FOR THE GRAINMEM ARRAY
	x->cloud = (cm_cloud *)sysmem_newptrclear((x->cloudsize) * sizeof(cm_cloud));
	if (x->cloud == NULL) {
		object_error((t_object *)x, "out of memory");
		return NULL;
	}
	
	// ALLOCATE MEMORY FOR THE GRAIN ARRAY IN EACH MEMBER OF THE GRAINMEM STRUCT
	for (i = 0; i < x->cloudsize; i++) {
		x->cloud[i].left = (double *)sysmem_newptrclear(((x->grainlength * x->m_sr) * MAX_PITCH) * sizeof(double));
		if (x->cloud[i].left == NULL) {
			object_error((t_object *)x, "out of memory");
			return NULL;
		}
		x->cloud[i].right = (double *)sysmem_newptrclear(((x->grainlength * x->m_sr) * MAX_PITCH) * sizeof(double));
		if (x->cloud[i].right == NULL) {
			object_error((t_object *)x, "out of memory");
			return NULL;
		}
	}
	
	// ALLOCATE MEMORY FOR PITCH LIST
	x->pitchlist = (double *)sysmem_newptrclear(PITCHLIST * sizeof(double));

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
	x->object_inlets[10] = 4.0; // initialize value for min alpha
	x->object_inlets[11] = 4.0; // initialize value for max alpha
	x->tr_prev = 0.0; // initialize value for previous trigger sample
	x->grains_count = 0; // initialize the grains count value
	x->buffer_modified = false; // initialize buffer modified flag

	// calculate constants for panning function
	x->piovr2 = 4.0 * atan(1.0) * 0.5;
	x->root2ovr2 = sqrt(2.0) * 0.5;

	// bang trigger flag
	x->bang_trigger = false;
	
	// pitchlist values
	x->pitchlist_active = false;
	x->pitchlist_zero = 0.0;
	x->pitchlist_size = 0.0;
	
	// cloud structure members
	for (i = 0; i < x->cloudsize; i++) {
		x->cloud[i].length = 0;
		x->cloud[i].pos = 0;
		x->cloud[i].busy = false;
	}
	
	x->cloudsize_new = x->cloudsize;
	x->grainlength_new = x->grainlength;
	
	x->resize_request = false;
	x->length_request = false;
	
	x->resize_verify = false;
	x->length_verify = false;

	/************************************************************************************************************************/
	// BUFFER REFERENCES
	x->buffer = buffer_ref_new((t_object *)x, x->buffer_name); // write the buffer reference into the object structure

	#ifdef WIN_VERSION
		srand((unsigned int)clock());
	#endif

	return x;
}


/************************************************************************************************************************/
/* THE 64 BIT DSP METHOD                                                                                                */
/************************************************************************************************************************/
void cmgausscloud_dsp64(t_cmgausscloud *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags) {
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
	x->connect_status[10] = count[11]; // 12th inlet: write connection flag into object structure (1 if signal connected)
	x->connect_status[11] = count[12]; // 13th inlet: write connection flag into object structure (1 if signal connected)

	if (x->m_sr != samplerate * 0.001) { // check if sample rate stored in object structure is the same as the current project sample rate
		x->m_sr = samplerate * 0.001;
		for (i = 0; i < x->cloudsize; i++) {
			x->cloud[i].left = (double *)sysmem_resizeptrclear(x->cloud[i].left, ((x->grainlength * x->m_sr) * MAX_PITCH) * sizeof(double));
			if (x->cloud[i].left == NULL) {
				object_error((t_object *)x, "out of memory");
				return;
			}
		}
		for (i = 0; i < x->cloudsize; i++) {
			x->cloud[i].right = (double *)sysmem_resizeptrclear(x->cloud[i].right, ((x->grainlength * x->m_sr) * MAX_PITCH) * sizeof(double));
			if (x->cloud[i].right == NULL) {
				object_error((t_object *)x, "out of memory");
				return;
			}
		}
	}

	// CALL THE PERFORM ROUTINE
	object_method(dsp64, gensym("dsp_add64"), x, cmgausscloud_perform64, 0, NULL);
	//	dsp_add64(dsp64, (t_object*)x, (t_perfroutine64)cmgausscloud_perform64, 0, NULL);
}


/************************************************************************************************************************/
/* THE 64 BIT PERFORM ROUTINE                                                                                           */
/************************************************************************************************************************/
void cmgausscloud_perform64(t_cmgausscloud *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam) {
	// VARIABLE DECLARATIONS
	t_bool trigger = false; // trigger occurred yes/no
	long i, r; // for loop counters
	long n = sampleframes; // number of samples per signal vector
	double tr_curr; // current trigger value
	double distance; // floating point index for reading from buffers
	double b_read, w_read; // current sample read from the sample buffer and window array
	double outsample_left = 0.0; // temporary left output sample used for adding up all grain samples
	double outsample_right = 0.0; // temporary right output sample used for adding up all grain samples
	int slot = 0; // variable for the current slot in the arrays to write grain info to
	cm_panstruct panstruct; // struct for holding the calculated constant power left and right stereo values
	// grain generation variables
	long readpos;
	long start;
	long smp_length;
	long pitch_length;
	double gain;
	double pan_left, pan_right;
	double alpha;
	
	// OUTLETS
	t_double *out_left 	= (t_double *)outs[0]; // assign pointer to left output
	t_double *out_right = (t_double *)outs[1]; // assign pointer to right output
	
	// BUFFER VARIABLE DECLARATIONS
	t_buffer_obj *buffer = buffer_ref_getobject(x->buffer);
	float *b_sample = buffer_locksamples(buffer);
	long b_framecount; // number of frames in the sample buffer
	t_atom_long b_channelcount; // number of channels in the sample buffer
	double b_m_sr; // buffer sample rate
	double sr_ratio; // ratio between buffer sample rate and system sample rate
	
	// CLOUDSIZE - MEMORY RESIZE
	if (x->grains_count == 0 && x->resize_request) {
		// allocate new memory and check if all went well
		x->resize_verify = cmgausscloud_resize(x);
		if (x->resize_verify) { // if all OK
			x->resize_verify = false;
			x->resize_request = false;
		}
		else {
			// if mem-allocation fails, go to zero and try again next time:
			// x->resize_request is not reset
			goto zero;
		}
	}
	
	// CLOUDSIZE - GRAIN LENGTH
	if (x->grains_count == 0 && x->length_request) {
		// allocate new memory and check if all went well
		x->length_verify = cmgausscloud_resize(x);
		if (x->length_verify) { // if all OK
			x->length_verify = false;
			x->length_request = false;
		}
		else {
			// if mem-allocation fails, go to zero and try again next time:
			// x->length_request is not reset
			goto zero;
		}
	}
	
	if (x->grains_count == 0 && x->buffer_modified) {
		x->buffer_modified = false;
	}

	// BUFFER CHECKS
	if (!b_sample) { // if the sample buffer does not exist
		goto zero;
	}

	// GET BUFFER INFORMATION
	b_framecount = buffer_getframecount(buffer); // get number of frames in the sample buffer
	b_channelcount = buffer_getchannelcount(buffer); // get number of channels in the sample buffer
	b_m_sr = buffer_getsamplerate(buffer) * 0.001; // get the sample buffer sample rate
	sr_ratio = b_m_sr / x->m_sr; // calculate ratio between system sample rate and buffer sample rate

	// GET INLET VALUES
	t_double *tr_sigin 	= (t_double *)ins[0]; // get trigger input signal from 1st inlet

	x->grain_params[0] = x->connect_status[0] ? *ins[1] * b_m_sr : x->object_inlets[0] * b_m_sr;	// start min
	x->grain_params[1] = x->connect_status[1] ? *ins[2] * b_m_sr : x->object_inlets[1] * b_m_sr;	// start max
	x->grain_params[2] = x->connect_status[2] ? *ins[3] * x->m_sr : x->object_inlets[2] * x->m_sr;	// length min
	x->grain_params[3] = x->connect_status[3] ? *ins[4] * x->m_sr : x->object_inlets[3] * x->m_sr;	// length max
	x->grain_params[4] = x->connect_status[4] ? *ins[5] : x->object_inlets[4];						// pitch min
	x->grain_params[5] = x->connect_status[5] ? *ins[6] : x->object_inlets[5];						// pitch max
	x->grain_params[6] = x->connect_status[6] ? *ins[7] : x->object_inlets[6];						// pan min
	x->grain_params[7] = x->connect_status[7] ? *ins[8] : x->object_inlets[7];						// pan max
	x->grain_params[8] = x->connect_status[8] ? *ins[9] : x->object_inlets[8];						// gain min
	x->grain_params[9] = x->connect_status[9] ? *ins[10] : x->object_inlets[9];						// gain max
	x->grain_params[10] = x->connect_status[10] ? *ins[11] : x->object_inlets[10];					// alpha min
	x->grain_params[11] = x->connect_status[11] ? *ins[12] : x->object_inlets[11];					// alpha max


	// DSP LOOP
	while (n--) {
		tr_curr = *tr_sigin++; // get current trigger value

		if (x->attr_zero) {
			if (signbit(tr_curr) != signbit(x->tr_prev)) { // zero crossing from negative to positive
				trigger = true;
			}
			else if (x->bang_trigger) {
				trigger = true;
				x->bang_trigger = false;
			}
		}
		else {
			if ((x->tr_prev - tr_curr) > 0.9) {
				trigger = true;
			}
			else if (x->bang_trigger) {
				trigger = true;
				x->bang_trigger = false;
			}
		}
		
		/************************************************************************************************************************/
		// IN CASE OF TRIGGER, LIMIT NOT MODIFIED AND GRAINS COUNT IN THE LEGAL RANGE (AVAILABLE SLOTS)
		if (trigger && x->grains_count < x->cloudsize && !x->resize_request && !x->length_request && !x->buffer_modified && b_sample) {
			trigger = false; // reset trigger
			x->grains_count++; // increment grains_count
			// FIND A FREE SLOT FOR THE NEW GRAIN
			i = 0;
			while (i < x->cloudsize) {
				if (!x->cloud[i].busy) {
					x->cloud[i].busy = true;
					slot = i;
					break;
				}
				i++;
			}

			
			for (i = 0; i < 6; i++) {
				// if currently processing randomized value for pitch (i == 2) and if pitchlist is active
				if (i == 2 && x->pitchlist_active) {
					// get random postition from pitchlist and write stored value
					x->randomized[i] = x->pitchlist[(int)cm_random(&x->pitchlist_zero, &x->pitchlist_size)];
				}
				else {
					r = i * 2;
					x->randomized[i] = cm_random(&x->grain_params[r], &x->grain_params[r+1]);
				}
			}
			
			// check for parameter sanity of the length value
			if (x->randomized[1] < MIN_GRAINLENGTH * x->m_sr) {
				x->randomized[1] = MIN_GRAINLENGTH * x->m_sr;
			}
			else if (x->randomized[1] > x->grainlength * x->m_sr) {
				x->randomized[1] = x->grainlength * x->m_sr;
			}
			
			// adjust the pitch value according to the sample rate ratio system/buffer
			x->randomized[2] = x->randomized[2] * sr_ratio;

			// check for parameter sanity of the pitch value
			if (x->randomized[2] < MIN_PITCH) {
				x->randomized[2] = MIN_PITCH;
			}
			else if (x->randomized[2] > MAX_PITCH) {
				x->randomized[2] = MAX_PITCH;
			}

			// check for parameter sanity of the pan value
			if (x->randomized[3] < MIN_PAN) {
				x->randomized[3] = MIN_PAN;
			}
			else if (x->randomized[3] > MAX_PAN) {
				x->randomized[3] = MAX_PAN;
			}

			// check for parameter sanity of the gain value
			if (x->randomized[4] < MIN_GAIN) {
				x->randomized[4] = MIN_GAIN;
			}
			else if (x->randomized[4] > MAX_GAIN) {
				x->randomized[4] = MAX_GAIN;
			}

			// check for parameter sanity of the alpha value
			if (x->randomized[5] < MIN_ALPHA) {
				x->randomized[5] = MIN_ALPHA;
			}
			else if (x->randomized[5] > MAX_ALPHA) {
				x->randomized[5] = MAX_ALPHA;
			}

			// write grain lenght slot (non-pitch)
			smp_length = x->randomized[1];
			pitch_length = smp_length * x->randomized[2]; // length * pitch
			// check that grain length is not larger than size of buffer
			if (pitch_length > b_framecount) {
				pitch_length = b_framecount;
			}
			x->cloud[slot].length = smp_length;
			
			// write start position
			start = x->randomized[0];
			// start position sanity testing
			if (start > b_framecount - pitch_length) {
				start = b_framecount - pitch_length;
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
			// write alpha value
			alpha = x->randomized[5];
			
			
			for (readpos = 0; readpos < smp_length; readpos++) { // if the current slot contains grain playback information
				// GET WINDOW SAMPLE FROM WINDOW BUFFER
				w_read = cm_gauss(&readpos, &smp_length, &alpha);
				
				// GET GRAIN SAMPLE FROM SAMPLE BUFFER
				distance = start + (((double)readpos / (double)smp_length) * (double)pitch_length);
				
				if (b_channelcount > 1 && x->attr_stereo) { // if more than one channel
					if (x->attr_sinterp) {
						// get interpolated sample
						x->cloud[slot].left[readpos] = ((cm_lininterp(distance, b_sample, b_channelcount, b_framecount, 0) * w_read) * pan_left) * gain;
						x->cloud[slot].right[readpos] = ((cm_lininterp(distance, b_sample, b_channelcount, b_framecount, 1) * w_read) * pan_right) * gain;
					}
					else {
						x->cloud[slot].left[readpos] = ((b_sample[(long)distance * b_channelcount] * w_read) * pan_left) * gain;
						x->cloud[slot].right[readpos] = ((b_sample[((long)distance * b_channelcount) + 1] * w_read) * pan_right) * gain;
					}
				}
				else {
					if (x->attr_sinterp) {
						b_read = cm_lininterp(distance, b_sample, b_channelcount, b_framecount, 0) * w_read; // get interpolated sample
						x->cloud[slot].left[readpos] = (b_read * pan_left) * gain;
						x->cloud[slot].right[readpos] = (b_read * pan_right) * gain;
					}
					else {
						x->cloud[slot].left[readpos] = ((b_sample[(long)distance * b_channelcount] * w_read) * pan_left) * gain;
						x->cloud[slot].right[readpos] = ((b_sample[(long)distance * b_channelcount] * w_read) * pan_right) * gain;
					}
				}
			}
		}
		/************************************************************************************************************************/
		// CONTINUE WITH THE PLAYBACK ROUTINE
		
		// playback only if there are grains to play
		if (x->grains_count) {
			for (i = 0; i < x->cloudsize; i++) {
				if (x->cloud[i].busy) {
					r = x->cloud[i].pos++;
					outsample_left += x->cloud[i].left[r];
					outsample_right += x->cloud[i].right[r];
					if (x->cloud[i].pos == x->cloud[i].length) {
						x->cloud[i].pos = 0;
						x->cloud[i].busy = false;
						x->grains_count--;
						if (x->grains_count < 0) {
							x->grains_count = 0;
						}
					}
				}
			}
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
void cmgausscloud_assist(t_cmgausscloud *x, void *b, long msg, long arg, char *dst) {
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
			case 11:
				snprintf_zero(dst, 256, "(signal/float) alpha min");
				break;
			case 12:
				snprintf_zero(dst, 256, "(signal/float) alpha max");
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
void cmgausscloud_free(t_cmgausscloud *x) {
	int i;
	dsp_free((t_pxobject *)x); // free memory allocated for the object
	object_free(x->buffer); // free the buffer reference
	
	for (i = 0; i < x->cloudsize; i++) {
		sysmem_freeptr(x->cloud[i].left);
		sysmem_freeptr(x->cloud[i].right);
	}
	sysmem_freeptr(x->cloud);
	
	sysmem_freeptr(x->object_inlets); // free memory allocated to the object inlets array
	sysmem_freeptr(x->grain_params); // free memory allocated to the grain parameters array
	sysmem_freeptr(x->randomized); // free memory allocated to the grain parameters array
}

/************************************************************************************************************************/
/* FLOAT METHOD FOR FLOAT INLET SUPPORT                                                                                 */
/************************************************************************************************************************/
void cmgausscloud_float(t_cmgausscloud *x, double f) {
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
			else if (f > x->grainlength) {
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
			else if (f > x->grainlength) {
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
		case 11:
			if (f < MIN_ALPHA || f > MAX_ALPHA) {
				dump = f;
			}
			else {
				x->object_inlets[10] = f;
			}
			break;
		case 12:
			if (f < MIN_ALPHA || f > MAX_ALPHA) {
				dump = f;
			}
			else {
				x->object_inlets[11] = f;
			}
			break;
	}
}


/************************************************************************************************************************/
/* DOUBLE CLICK METHOD FOR VIEWING BUFFER CONTENT                                                                       */
/************************************************************************************************************************/
void cmgausscloud_dblclick(t_cmgausscloud *x) {
	buffer_view(buffer_ref_getobject(x->buffer));
}


/************************************************************************************************************************/
/* NOTIFY METHOD FOR THE BUFFER REFERENCES                                                                              */
/************************************************************************************************************************/
t_max_err cmgausscloud_notify(t_cmgausscloud *x, t_symbol *s, t_symbol *msg, void *sender, void *data) {
	if (msg == ps_buffer_modified) {
		x->buffer_modified = true;
	}
	return buffer_ref_notify(x->buffer, s, msg, sender, data); // return with the calling buffer
}


/************************************************************************************************************************/
/* THE ACTUAL BUFFER SET METHOD                                                                                         */
/************************************************************************************************************************/
void cmgausscloud_doset(t_cmgausscloud *x, t_symbol *s, long ac, t_atom *av) {
	if (ac == 1) {
		x->buffer_modified = true;
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
void cmgausscloud_set(t_cmgausscloud *x, t_symbol *s, long ac, t_atom *av) {
	defer(x, (method)cmgausscloud_doset, s, ac, av);
}


/************************************************************************************************************************/
/* THE RESIZE REQUEST METHOD                                                                                            */
/************************************************************************************************************************/
void cmgausscloud_cloudsize(t_cmgausscloud *x, t_symbol *s, long ac, t_atom *av) {
	long arg = atom_getlong(av);
	if (ac && av) {
		if (arg < 1) {
			object_error((t_object *)x, "cloud size must be larger than 1");
		}
		else {
			x->cloudsize_new = arg;
			x->resize_request = true;
		}
	}
	else {
		object_error((t_object *)x, "argument required (cloud size)");
	}
}


/************************************************************************************************************************/
/* THE GRAINLENGTH REQUEST METHOD                                                                                       */
/************************************************************************************************************************/
void cmgausscloud_grainlength(t_cmgausscloud *x, t_symbol *s, long ac, t_atom *av) {
	long arg = atom_getlong(av);
	if (ac && av) {
		if (arg < MIN_GRAINLENGTH) {
			object_error((t_object *)x, "max. grain length must be larger than %d", MIN_GRAINLENGTH);
		}
		else {
			x->grainlength_new = arg;
			x->length_request = true;
		}
	}
	else {
		object_error((t_object *)x, "argument required (max. grain length)");
	}
}


/************************************************************************************************************************/
/* THE ACTUAL RESIZE METHOD                                                                                             */
/************************************************************************************************************************/
t_bool cmgausscloud_resize(t_cmgausscloud *x) {
	int i;
	
	for (i = 0; i < x->cloudsize; i++) {
		sysmem_freeptr(x->cloud[i].left);
		sysmem_freeptr(x->cloud[i].right);
	}
	sysmem_freeptr(x->cloud);
	
	if (x->resize_request) {
		x->cloudsize = x->cloudsize_new;
	}
	else if (x->length_request) {
		x->grainlength = x->grainlength_new;
	}
	
	// ALLOCATE MEMORY FOR THE GRAINMEM ARRAY
	x->cloud = (cm_cloud *)sysmem_newptrclear((x->cloudsize) * sizeof(cm_cloud));
	if (x->cloud == NULL) {
		object_error((t_object *)x, "out of memory");
		x->resize_verify = false;
		return false;
	}
	
	// ALLOCATE MEMORY FOR THE GRAIN ARRAY IN EACH MEMBER OF THE GRAINMEM STRUCT
	for (i = 0; i < x->cloudsize; i++) {
		x->cloud[i].left = (double *)sysmem_newptrclear(((x->grainlength * x->m_sr) * MAX_PITCH) * sizeof(double));
		if (x->cloud[i].left == NULL) {
			object_error((t_object *)x, "out of memory");
			x->resize_verify = false;
			return false;
		}
		x->cloud[i].right = (double *)sysmem_newptrclear(((x->grainlength * x->m_sr) * MAX_PITCH) * sizeof(double));
		if (x->cloud[i].right == NULL) {
			object_error((t_object *)x, "out of memory");
			x->resize_verify = false;
			return false;
		}
	}
	
	return true;
}


/************************************************************************************************************************/
/* THE PITCHLIST METHOD                                                                                                 */
/************************************************************************************************************************/
void cmgausscloud_pitchlist(t_cmgausscloud *x, t_symbol *s, long ac, t_atom *av) {
	double value;
	if (ac < 1) {
		object_error((t_object *)x, "minimum number of pitch values is 1");
	}
	else if (ac == 1 && atom_getfloat(av) == 0) {
		x->pitchlist_active = false;
	}
	else if (ac <= 10) {
		x->pitchlist_active = true;
		// clear array
		for (int i = 0; i < PITCHLIST; i++) {
			x->pitchlist[i] = 0;
		}
		x->pitchlist_size = (double)ac;
		// write args into array
		for (int i = 0; i < x->pitchlist_size; i++) {
			value = atom_getfloat(av+i);
			if (value > MAX_PITCH) {
				object_error((t_object *)x, "value of element %d (%.3f) must not be higher than %d - setting value to %d", (i+1), value, MAX_PITCH, MAX_PITCH);
				value = MAX_PITCH;
			}
			else if (value < MIN_PITCH) {
				object_error((t_object *)x, "value of element %d (%.3f) must be higher than %.3f - setting value to %.3f", (i+1), value, MIN_PITCH, MIN_PITCH);
				value = MIN_PITCH;
			}
			x->pitchlist[i] = value;
		}
	}
	else {
		object_error((t_object *)x, "maximum number of pitch values is 10");
	}
}


/************************************************************************************************************************/
/* THE BANG METHOD                                                                                                      */
/************************************************************************************************************************/
void cmgausscloud_bang(t_cmgausscloud *x) {
	x->bang_trigger = true;
}


/************************************************************************************************************************/
/* THE STEREO ATTRIBUTE SET METHOD                                                                                      */
/************************************************************************************************************************/
t_max_err cmgausscloud_stereo_set(t_cmgausscloud *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_stereo = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}


/************************************************************************************************************************/
/* THE SAMPLE INTERPOLATION ATTRIBUTE SET METHOD                                                                        */
/************************************************************************************************************************/
t_max_err cmgausscloud_sinterp_set(t_cmgausscloud *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_sinterp = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}


/************************************************************************************************************************/
/* THE ZERO CROSSING ATTRIBUTE SET METHOD                                                                               */
/************************************************************************************************************************/
t_max_err cmgausscloud_zero_set(t_cmgausscloud *x, t_object *attr, long ac, t_atom *av) {
	if (ac && av) {
		x->attr_zero = atom_getlong(av)? 1 : 0;
	}
	return MAX_ERR_NONE;
}


/************************************************************************************************************************/
/* CUSTOM FUNCTIONS																										*/
/************************************************************************************************************************/
// constant power stereo function
void cm_panning(cm_panstruct *panstruct, double *pos, t_cmgausscloud *x) {
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
double cm_lininterp(double distance, float *buffer, t_atom_long b_channelcount, t_atom_long b_framecount, short channel) {
	long index = (long)distance; // get truncated index
	long next = index + 1;
	if (next > b_framecount) {
		next = 0;
	}
	distance -= (long)distance; // calculate fraction value for interpolation
	return buffer[index * b_channelcount + channel] + distance * (buffer[next * b_channelcount + channel] - buffer[index * b_channelcount + channel]);
}
// GAUSS WINDOW FUNCTION
double cm_gauss(long *pos, long *length, double *alpha) {
	double n;
	double N = *length - 1;
	double stdev = N / (2 * (*alpha));
	n = *pos - N / 2;
	return exp(-0.5 * pow((n / stdev), 2));
}
