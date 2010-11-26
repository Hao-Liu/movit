#include <stdio.h>
#include <cv.h>

int openTIFF(char *filename, FILE** fp, int *numFrame, int *first);
int closeTIFF(FILE* fp);
int	readTIFF(FILE *fp,int *idx,int nFrame, IplImage ***img);
int releaseTIFF(IplImage ***img, int nFrame);

