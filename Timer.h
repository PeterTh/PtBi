#pragma once

#include "stdafx.h"

class Timer {
	LARGE_INTEGER startTime;
	static double multiplicator;

public:
	Timer() {
		if(multiplicator<0) {
			LARGE_INTEGER frequency;
			RT_ASSERT(QueryPerformanceFrequency(&frequency)!=0, "Could not get HPC frequency (not supported?).");
			multiplicator = 1000000.0/(double)frequency.QuadPart; // microseconds
		}
		start();
	}

	void start() {
		QueryPerformanceCounter(&startTime);
	}

	double elapsed() {
		LARGE_INTEGER time, diff;
		QueryPerformanceCounter(&time);
		diff.QuadPart = time.QuadPart - startTime.QuadPart;
		return (double)(diff.QuadPart)*multiplicator;
	}

	void moveStart(LONGLONG amount) {
		startTime.QuadPart += (LONGLONG)(amount/multiplicator);
	}
};

class SlidingAverage {
	double *vals;
	unsigned interval, current;
	bool filled;
public:
	SlidingAverage(unsigned interval) : interval(interval), current(0), filled(false) {
		vals = new double[interval];
	}
	~SlidingAverage() {
		delete vals;
	}

	void add(double value) {
		if(++current == interval) {
			current = 0;
			filled = true;
		} else {
			filled = false;
		}
		vals[current] = value;
	}

	double get() {
		double sum = 0;
		for(unsigned i=0; i<interval; ++i) { sum += vals[i]; }
		sum /= interval;
		return sum;
	}

	bool justFilled() {
		return filled;
	}
};
