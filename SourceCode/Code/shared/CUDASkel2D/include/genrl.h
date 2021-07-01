#pragma once


const float MYINFINITY = 1.0e7f;
const float eps  	 = 1.0e-6f;
const float eps2 	 = eps*eps;
const int   CONN_UNKNOWN = -1;


#define CHECKZ(x,y) ((fabs(y)>1.0e-8)? (x/y) : 0)


inline float SQR(float x) 			{ return x*x; }
inline int   SQR(int x)                 	{ return x*x; }
inline float max(float x,float y) 		{ return (x<y)? y : x; }
inline float min(float x,float y)		{ return (x<y)? x : y; } 
inline float INTERP(float a,float b,float t)	{ return a*(1-t)+b*t;  }



struct Coord { 
		int i,j; 
		Coord(int i_,int j_):i(i_),j(j_) {}; 
		Coord() {} 
		int operator==(const Coord& c) const { return i==c.i && j==c.j; } 
	      };

