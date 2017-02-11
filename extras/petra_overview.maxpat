{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 7,
			"minor" : 3,
			"revision" : 1,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"rect" : [ 468.0, 155.0, 976.0, 732.0 ],
		"bglocked" : 0,
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 1,
		"objectsnaponopen" : 1,
		"statusbarvisible" : 2,
		"toolbarvisible" : 1,
		"lefttoolbarpinned" : 0,
		"toptoolbarpinned" : 0,
		"righttoolbarpinned" : 0,
		"bottomtoolbarpinned" : 0,
		"toolbars_unpinned_last_save" : 15,
		"tallnewobj" : 0,
		"boxanimatetime" : 200,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"description" : "",
		"digest" : "",
		"tags" : "",
		"style" : "",
		"subpatcher_template" : "",
		"boxes" : [ 			{
				"box" : 				{
					"bgcolor" : [ 0.145098, 0.352941, 0.670588, 1.0 ],
					"fontface" : 1,
					"fontsize" : 18.0,
					"id" : "obj-9",
					"maxclass" : "textbutton",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "int" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 772.0, 93.208183, 193.0, 61.291817 ],
					"rounded" : 8.0,
					"style" : "",
					"text" : "general granulation principles",
					"texton" : "general granulation principles",
					"textoncolor" : [ 1.0, 1.0, 1.0, 1.0 ],
					"truncate" : 0
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-10",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 772.0, 222.326126, 35.0, 22.0 ],
					"style" : "",
					"text" : "front"
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-11",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 7,
							"minor" : 3,
							"revision" : 1,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"rect" : [ 559.0, 266.0, 704.0, 420.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 15,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"boxes" : [ 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"id" : "obj-11",
									"linecount" : 18,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 14.0, 146.0, 677.0, 248.0 ],
									"style" : "",
									"text" : "DSP processing of the petra granulation objects consists of two parts. The first part listens for new triggers, generates new grains and stores them in memory. The second part checks if there are grains contained in memory and continues with grain playback.\n\nAll objects use audio signals as triggers. By default, they take a signal ramp from 0 to 1 (such as the signal produced by the phasor~ object) and trigger a grain each time the signal falls back to 0. Grain triggering thus occurs with sample precision.\n\nWhen a trigger occurs, the object calculates the grain parameters based on the values provided through the object inlets, generates a single grain, allocates it a slot and writes it into an internal buffer (memory). The size of this buffer is based on the cloud size argument provided when you load the object. For example, when you specify a cloud size of 32 grains, the object allocates memory to store 32 grains. For each grain, the object stores the grain length and the respective playback position throughout the duration of the grain.\n\nAs soon as trigger detection and grain generation is completed, the object continues with grain playback. The playback section checks for existing grains and starts playback of new grains, or continues playback of existing grains. When a grain has finished playing, the slot in memory is freed for new grains and can be overwritten with a new grain.\n\nAfter the playback section, DSP processing restarts and lsitens for new triggers."
								}

							}
, 							{
								"box" : 								{
									"fontface" : 3,
									"fontsize" : 36.0,
									"id" : "obj-3",
									"linecount" : 2,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 231.0, 25.288361, 378.0, 87.0 ],
									"style" : "",
									"text" : "general granulation principles"
								}

							}
, 							{
								"box" : 								{
									"autofit" : 1,
									"forceaspect" : 1,
									"id" : "obj-4",
									"maxclass" : "fpic",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "jit_matrix" ],
									"patching_rect" : [ 14.0, 5.0, 200.0, 200.0 ],
									"pic" : "maxOverviewIcon.png"
								}

							}
, 							{
								"box" : 								{
									"hidden" : 1,
									"id" : "obj-2",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 622.0, 61.288361, 69.0, 22.0 ],
									"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ],
									"style" : "",
									"text" : "thispatcher"
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"hidden" : 1,
									"id" : "obj-1",
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 622.0, 25.288361, 30.0, 30.0 ],
									"style" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"disabled" : 0,
									"hidden" : 1,
									"source" : [ "obj-1", 0 ]
								}

							}
 ]
					}
,
					"patching_rect" : [ 772.0, 248.826141, 179.0, 22.0 ],
					"saved_object_attributes" : 					{
						"description" : "",
						"digest" : "",
						"globalpatchername" : "",
						"style" : "",
						"tags" : ""
					}
,
					"style" : "",
					"text" : "p general-granulation-principles"
				}

			}
, 			{
				"box" : 				{
					"fontsize" : 13.0,
					"id" : "obj-7",
					"linecount" : 2,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 30.0, 315.826141, 929.0, 36.0 ],
					"style" : "",
					"text" : "In addition, petra contains an audio object for live input granulation. It makes use of a circular buffer and an adjustable, and optionally randomised, delay control over the duration of the entire buffer."
				}

			}
, 			{
				"box" : 				{
					"autofit" : 1,
					"forceaspect" : 1,
					"id" : "obj-3",
					"maxclass" : "fpic",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "jit_matrix" ],
					"patching_rect" : [ 30.0, 15.0, 633.0, 225.56138 ],
					"pic" : "petra-logo.png"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.145098, 0.352941, 0.670588, 1.0 ],
					"fontsize" : 14.0,
					"id" : "obj-14",
					"maxclass" : "textbutton",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "int" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 513.0, 649.576721, 219.0, 31.0 ],
					"rounded" : 8.0,
					"style" : "",
					"text" : "open the cm.livecloud~ helpfile",
					"texton" : "open the cm.livecloud~ helpfile",
					"textoncolor" : [ 1.0, 1.0, 1.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-18",
					"ignoreclick" : 1,
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 513.0, 682.576721, 142.0, 22.0 ],
					"style" : "",
					"text" : "zl.reg help cm.livecloud~"
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-19",
					"ignoreclick" : 1,
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 513.0, 706.576721, 52.0, 22.0 ],
					"style" : "",
					"text" : "pcontrol"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 0,
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-20",
					"linecount" : 3,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 513.0, 585.576721, 430.0, 47.0 ],
					"style" : "",
					"text" : "Polyphonic granulator object that records incoming audio into an internal circular buffer. Any signal can be granulated in real time with adjustable delay. The object uses a windowing function loaded into a buffer~ object."
				}

			}
, 			{
				"box" : 				{
					"fontface" : 3,
					"fontsize" : 20.0,
					"id" : "obj-21",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 513.0, 554.576721, 430.0, 29.0 ],
					"style" : "",
					"text" : "cm.livecloud~"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.145098, 0.352941, 0.670588, 1.0 ],
					"fontface" : 1,
					"fontsize" : 18.0,
					"id" : "obj-68",
					"maxclass" : "textbutton",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "int" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 772.0, 15.0, 193.0, 62.355873 ],
					"rounded" : 8.0,
					"style" : "",
					"text" : "common object properties",
					"texton" : "common object properties",
					"textoncolor" : [ 1.0, 1.0, 1.0, 1.0 ],
					"truncate" : 0
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-31",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 772.0, 169.561386, 35.0, 22.0 ],
					"style" : "",
					"text" : "front"
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-29",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 7,
							"minor" : 3,
							"revision" : 1,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"rect" : [ 549.0, 228.0, 713.0, 587.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 15,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"boxes" : [ 							{
								"box" : 								{
									"fontface" : 3,
									"fontsize" : 36.0,
									"id" : "obj-3",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 231.0, 47.288361, 470.0, 47.0 ],
									"style" : "",
									"text" : "common object properties"
								}

							}
, 							{
								"box" : 								{
									"autofit" : 1,
									"forceaspect" : 1,
									"id" : "obj-4",
									"maxclass" : "fpic",
									"numinlets" : 1,
									"numoutlets" : 1,
									"outlettype" : [ "jit_matrix" ],
									"patching_rect" : [ 14.0, 5.0, 200.0, 200.0 ],
									"pic" : "maxOverviewIcon.png"
								}

							}
, 							{
								"box" : 								{
									"hidden" : 1,
									"id" : "obj-2",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 603.0, 186.0, 69.0, 22.0 ],
									"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ],
									"style" : "",
									"text" : "thispatcher"
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"hidden" : 1,
									"id" : "obj-1",
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 603.0, 150.0, 30.0, 30.0 ],
									"style" : ""
								}

							}
, 							{
								"box" : 								{
									"fontface" : 1,
									"fontsize" : 14.0,
									"id" : "obj-12",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 150.0, 527.0, 22.0 ],
									"style" : "",
									"text" : "Minimum and maximum values:"
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"id" : "obj-21",
									"linecount" : 8,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 302.0, 174.0, 67.0, 114.0 ],
									"style" : "",
									"text" : "1 ms\n500 ms\n0.001\n4\n0\n2\n0.1\n10",
									"textjustification" : 2
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"id" : "obj-11",
									"linecount" : 8,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 174.0, 527.0, 114.0 ],
									"style" : "",
									"text" : "- Minimum grain length value:\n- Maximum grain length value:\n- Minimum pitch value:\n- Maximum pitch value:\n- Minimum gain value:\n- Maximum gain value:\n- Minimum alpha value (cm.gausscloud~ only):\n- Maximum alpha value (cm.gausscloud~ only):"
								}

							}
, 							{
								"box" : 								{
									"fontface" : 1,
									"fontsize" : 14.0,
									"id" : "obj-10",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 303.0, 668.0, 22.0 ],
									"style" : "",
									"text" : "Common object attributes:"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-9",
									"linecount" : 17,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 327.0, 686.0, 234.0 ],
									"style" : "",
									"text" : "@stereo (0 or 1)\nActivates/deactivates stereo playback when multi-channel files are loaded into the sample buffer. Default is 0. Not available for cm.livecloud~.\n\n@w_interp (0 or 1)\nActivates/deactivates window interpolation. Saves some CPU when deactivated.\nDefault is 0. Not available for cm.gausscloud~ because the windowing function is calculated in real time.\n\n@s_interp (0 or 1)\nActivates/deactivates sample interpolation. Saves some CPU when deactivated.\nDefault is 1.\n\n@zero (0 or 1)\nActivates/deactivates zero crossing trigger mode. When activated, a grain is triggered as soon as a zero crossing from < 0 to > 0 is detected (audio signals as trigger signals). When deactivated, a grain is triggered as soon as the signal generated by phasor~ wraps back from 1 to 0.\nDefault is 0."
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"disabled" : 0,
									"hidden" : 1,
									"source" : [ "obj-1", 0 ]
								}

							}
 ]
					}
,
					"patching_rect" : [ 772.0, 196.342072, 160.0, 22.0 ],
					"saved_object_attributes" : 					{
						"description" : "",
						"digest" : "",
						"globalpatchername" : "",
						"style" : "",
						"tags" : ""
					}
,
					"style" : "",
					"text" : "p common-object-properties"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.145098, 0.352941, 0.670588, 1.0 ],
					"fontsize" : 14.0,
					"id" : "obj-22",
					"maxclass" : "textbutton",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "int" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 513.0, 477.576721, 219.0, 31.0 ],
					"rounded" : 8.0,
					"style" : "",
					"text" : "open the cm.gausscloud~ helpfile",
					"texton" : "open the cm.gausscloud~ helpfile",
					"textoncolor" : [ 1.0, 1.0, 1.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-23",
					"ignoreclick" : 1,
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 513.0, 510.576721, 156.0, 22.0 ],
					"style" : "",
					"text" : "zl.reg help cm.gausscloud~"
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-26",
					"ignoreclick" : 1,
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 513.0, 534.576721, 52.0, 22.0 ],
					"style" : "",
					"text" : "pcontrol"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 0,
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-27",
					"linecount" : 4,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 513.0, 413.576721, 461.0, 60.0 ],
					"style" : "",
					"text" : "Polyphonic granulator object for granulation of mono and stereo audio files loaded into a buffer~ object. It uses a gaussian windowing function calculated inside the external itself. The shape of the gaussian window can be freely manipulated in real time and per grain with the min/max alpha value object inlets."
				}

			}
, 			{
				"box" : 				{
					"fontface" : 3,
					"fontsize" : 20.0,
					"id" : "obj-28",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 513.0, 382.576721, 430.0, 29.0 ],
					"style" : "",
					"text" : "cm.gausscloud~"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.145098, 0.352941, 0.670588, 1.0 ],
					"fontsize" : 14.0,
					"id" : "obj-2",
					"maxclass" : "textbutton",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "int" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 30.0, 649.576721, 219.0, 31.0 ],
					"rounded" : 8.0,
					"style" : "",
					"text" : "open the cm.indexcloud~ helpfile",
					"texton" : "open the cm.indexcloud~ helpfile",
					"textoncolor" : [ 1.0, 1.0, 1.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-5",
					"ignoreclick" : 1,
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 30.0, 682.576721, 153.0, 22.0 ],
					"style" : "",
					"text" : "zl.reg help cm.indexcloud~"
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-6",
					"ignoreclick" : 1,
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 30.0, 706.576721, 52.0, 22.0 ],
					"style" : "",
					"text" : "pcontrol"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 0,
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-16",
					"linecount" : 4,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 30.0, 585.576721, 471.0, 60.0 ],
					"style" : "",
					"text" : "Polyphonic granulator object for granulation of mono and stereo audio files loaded into a buffer~ object. It uses a windowing function calculated inside the external itself. The windowing functions can be accessed with and index number supplied as an argument or with a message."
				}

			}
, 			{
				"box" : 				{
					"fontface" : 3,
					"fontsize" : 20.0,
					"id" : "obj-17",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 30.0, 554.576721, 423.0, 29.0 ],
					"style" : "",
					"text" : "cm.indexcloud~"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 0,
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-15",
					"linecount" : 3,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 30.0, 413.576721, 433.0, 47.0 ],
					"style" : "",
					"text" : "Polyphonic granulator object for granulation of mono and stereo audio files loaded into a buffer~ object. It uses a windowing function loaded into a buffer~ object."
				}

			}
, 			{
				"box" : 				{
					"fontface" : 3,
					"fontsize" : 20.0,
					"id" : "obj-13",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 30.0, 382.576721, 421.0, 29.0 ],
					"style" : "",
					"text" : "cm.buffercloud~"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 13.0,
					"id" : "obj-25",
					"linecount" : 4,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 30.0, 248.826141, 925.0, 65.0 ],
					"style" : "",
					"text" : "2012 - 2017 by circuit.music.labs. A set of polyphonic granular synthesis objects.\n\npetra is a is a collection of external audio objects for polyphonic granulation of pre-recorded sounds. The package is loosely based on the principle of asynchronous granular synthesis. The objects are made for sample precision granulation of both single- and dual-channel audio files."
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.145098, 0.352941, 0.670588, 1.0 ],
					"fontsize" : 14.0,
					"id" : "obj-8",
					"maxclass" : "textbutton",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "", "", "int" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 30.0, 477.576721, 219.0, 31.0 ],
					"rounded" : 8.0,
					"style" : "",
					"text" : "open the cm.buffercloud~ helpfile",
					"texton" : "open the cm.buffercloud~ helpfile",
					"textoncolor" : [ 1.0, 1.0, 1.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-4",
					"ignoreclick" : 1,
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 30.0, 510.576721, 155.0, 22.0 ],
					"style" : "",
					"text" : "zl.reg help cm.buffercloud~"
				}

			}
, 			{
				"box" : 				{
					"hidden" : 1,
					"id" : "obj-24",
					"ignoreclick" : 1,
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 30.0, 534.576721, 52.0, 22.0 ],
					"style" : "",
					"text" : "pcontrol"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-11", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-10", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-18", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-14", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-19", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-5", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-23", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-22", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-26", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-23", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-29", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-31", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-24", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-4", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-6", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-5", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-31", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-68", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-8", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-10", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-9", 0 ]
				}

			}
 ],
		"dependency_cache" : [ 			{
				"name" : "maxOverviewIcon.png",
				"bootpath" : "~/Documents/MaxDeveloper/petra-dev/petra/media/images",
				"type" : "PNG ",
				"implicit" : 1
			}
, 			{
				"name" : "petra-logo.png",
				"bootpath" : "~/Documents/MaxDeveloper/petra-dev/petra/media/images",
				"type" : "PNG ",
				"implicit" : 1
			}
 ],
		"autosave" : 0
	}

}
