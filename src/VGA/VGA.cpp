/*
	Author: bitluni 2019
	License: 
	Creative Commons Attribution ShareAlike 2.0
	https://creativecommons.org/licenses/by-sa/2.0/
	
	For further details check out: 
		https://youtube.com/bitlunislab
		https://github.com/bitluni
		http://bitluni.net
*/
#include "VGA.h"

//maximum pixel clock with apll is 36249999.
//hfront hsync hback pixels vfront vsync vback lines divx divy pixelclock
const int VGA::MODE320x480[] = {16, 96, 52, 640, 11, 2, 31, 480, 2, 1, 25175000};
const int VGA::MODE320x240[] = {16, 96, 52, 640, 11, 2, 31, 480, 2, 2, 25175000};
const int VGA::MODE320x120[] = {16, 96, 52, 640, 11, 2, 31, 480, 2, 4, 25175000};
const int VGA::MODE320x400[] = {16, 96, 48, 640, 11, 2, 31, 400, 2, 1, 25175000};
const int VGA::MODE320x200[] = {16, 96, 48, 640, 11, 2, 31, 400, 2, 2, 25175000};
const int VGA::MODE320x100[] = {16, 96, 48, 640, 11, 2, 31, 400, 2, 4, 25175000};
const int VGA::MODE360x400[] = {16, 108, 56, 720, 11, 2, 32, 400, 2, 1, 28322000};
const int VGA::MODE360x200[] = {16, 108, 56, 720, 11, 2, 32, 400, 2, 2, 28322000};
const int VGA::MODE360x100[] = {16, 108, 56, 720, 11, 2, 32, 400, 2, 4, 28322000};
const int VGA::MODE360x350[] = {16, 108, 56, 720, 11, 2, 32, 350, 2, 1, 28322000};
const int VGA::MODE360x175[] = {16, 108, 56, 720, 11, 2, 32, 350, 2, 2, 28322000};
const int VGA::MODE360x88[] = {16, 108, 56, 720, 11, 2, 31, 350, 2, 4, 28322000};

//not supported on any of my screens
const int VGA::MODE384x576[] = {24, 80, 104, 768, 1, 3, 17, 576, 2, 1, 34960000};
const int VGA::MODE384x288[] = {24, 80, 104, 768, 1, 3, 17, 576, 2, 2, 34960000};
const int VGA::MODE384x144[] = {24, 80, 104, 768, 1, 3, 17, 576, 2, 4, 34960000};
const int VGA::MODE384x96[] = {24, 80, 104, 768, 1, 3, 17, 576, 2, 6, 34960000};

//not stable (can't reach 40MHz pixel clock, it's clipped by the driver to 36249999 at undivided resolution)
//you can mod the timings a bit the get it running on your system
const int VGA::MODE400x300[] = {40, 128, 88, 800, 1, 4, 23, 600, 2, 2, 39700000};
const int VGA::MODE400x150[] = {40, 128, 88, 800, 1, 4, 23, 600, 2, 4, 39700000};
const int VGA::MODE400x100[] = {40, 128, 88, 800, 1, 4, 23, 600, 2, 6, 39700000};
const int VGA::MODE200x150[] = {40, 128, 88, 800, 1, 4, 23, 600, 4, 4, 39700000};

//460 pixels horizontal it's based on 640x480
const int VGA::MODE460x480[] = {24, 136, 76, 920, 11, 2, 31, 480, 2, 1, 36249999};
const int VGA::MODE460x240[] = {24, 136, 76, 920, 11, 2, 31, 480, 2, 2, 36249999};
const int VGA::MODE460x120[] = {24, 136, 76, 920, 11, 2, 31, 480, 2, 4, 36249999};
const int VGA::MODE460x96[] = {24, 136, 76, 920, 11, 2, 31, 480, 2, 5, 36249999};

const int VGA::bytesPerSample = 2;

VGA::VGA(const int i2sIndex)
	: I2S(i2sIndex)
{
	lineBufferCount = 8;
}

bool VGA::init(const int *mode, const int *pinMap)
{
	int xres = mode[3] / mode[8];
	int yres = mode[7] / mode[9];
	hres = xres;
	propagateResolution(xres, yres);
	this->vsyncPin = vsyncPin;
	this->hsyncPin = hsyncPin;
	hdivider = mode[8];
	vdivider = mode[9];
	hfront = mode[0] / hdivider;
	hsync = mode[1] / hdivider;
	hback = mode[2] / hdivider;
	totalLines = mode[4] + mode[5] + mode[6] + mode[7];
	vfront = mode[4];
	vsync = mode[5];
	vback = mode[6];

	initParallelOutputMode(pinMap, mode[10] / hdivider);
	allocateLineBuffers(lineBufferCount);
	currentLine = 0;
	startTX();
	return true;
}

void VGA::setLineBufferCount(int lineBufferCount)
{
	this->lineBufferCount = lineBufferCount;
}

/// simple ringbuffer of blocks of size bytes each
void VGA::allocateLineBuffers(const int lines)
{
	dmaBufferCount = lines;
	dmaBuffers = (DMABuffer **)malloc(sizeof(DMABuffer *) * dmaBufferCount);
	if (!dmaBuffers)
		DEBUG_PRINTLN("Failed to allocate DMABuffer array");
	for (int i = 0; i < dmaBufferCount; i++)
	{
		dmaBuffers[i] = DMABuffer::allocate((hfront + hsync + hback + hres) * bytesPerSample); //front porch + hsync + back porch + pixels
		if (i)
			dmaBuffers[i - 1]->next(dmaBuffers[i]);
	}
	dmaBuffers[dmaBufferCount - 1]->next(dmaBuffers[0]);
}
