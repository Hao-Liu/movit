#include <cv.h>
typedef struct TRACKITEM
{
	CvKalman *kalman;
	double x;
	double y;
	double vx;
	double vy;
	double ax;
	double ay;
	double x_pre;
	double y_pre;
	double vx_pre;
	double vy_pre;
	double ax_pre;
	double ay_pre;
	double width;
	double height;
	double angle;
	int disappear;
	int index;
}TRACKITEM;

typedef struct TRACKSET
{
	CvSeq *head;
	CvSeq *current;
	CvSeq *tail;
}TRACKSET;
 
typedef struct TRACK
{
	CvSeq *track;
	int   index;
}TRACK;

int	getTrack(CvSeq **pos, int n, TRACKSET *trackSet, CvSeq *curTrack, CvMemStorage *storage);

