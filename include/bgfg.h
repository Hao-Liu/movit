#include <cv.h>
int getBackground(IplImage **img, int n, IplImage *bg);
int getForeground(IplImage **img, int n, IplImage *bg);
int getParameters(IplImage **img, int n, double *thr, double *dia, int *jit);
int getPosition(IplImage **img, int n, double thr, double dia, int jit, CvSeq **pos);


