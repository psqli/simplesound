=============
Digital audio
=============

:Date: 2018-07-06

This document explains how the digital audio is
represented and the meaning of some words.

The Analog to Digital Conversor (ADC) transforms voltage
at a point in time into a digital number. The Digital to
Analog Conversor (DAC) does the inverse.

This is an analog sound wave::

	                              .-.-.
	                           .-´     `-.
	                        .-´           `-.
	--.                  .-´                 `--
	   `-.             -´
	      `-.       .-´
	         `-.-.-´

Its digital representation is a series of amplitude
values along the time (known as samples)::

	 3|                                o
	 2|  sample                   o         o
	 1|   /
	 0|  o                   o                   o
	-1|
	-2|       o         o
	-3|            o
	     |    |    |    |    |    |    |    |    |

The sound device takes a **sample** of the sound wave at
equal time intervals. Multiple samples captured at the
same time make the channels. The samples from all the
channels are called a **frame**.

The number of frames per second is called **rate**.

The sample has a range limited by the number of bits
reserved to store it. In the example above, the samples
have a range from -3 to 3 (7 possibilities), thus 4 bits
(8 possibilities) is sufficient to store the data.
Generally the ranges are a multiple of a byte (8 bits).
