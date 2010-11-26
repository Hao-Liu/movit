/*
MOVIT: Analyse Particles' Motion Profile in Electrophoresis. 
*/
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#include "track.h"
#include <stdio.h>

#define TIFF_BLOCK_SIZE 100

int loop(char ** filename)
{
	FILE *fpImage=NULL;
	int nTotalFrame=0;
	int nBlockFrame=0;
	int idxFrame=0;
	int i=0;
	int j=0;

	int histRadii[2000]={0};
	int histDist[2000]={0};
	int histVel[2000]={0};

	int nJitter=0;
	double dThreshold=0.0;
	double dDiameter=0.0;
	
	IplImage **imgOrigin=NULL;
	IplImage  *imgBackground=NULL;
	IplImage *radii_hist_image=NULL;
	IplImage *dist_hist_image=NULL;
	IplImage *vel_hist_image=NULL;
	CvMemStorage* storage=NULL;
	CvSeq	**position=NULL;
	TRACKSET	trackSet;
	CvSeq	*currentTrack=NULL;
	
	if(!openTIFF(filename[1], &fpImage, &nTotalFrame, &idxFrame))
		exit(1);
//	printf("%d frames found in %s\n", nTotalFrame, filename[1]);							//debug
	
	storage=cvCreateMemStorage(0);
	currentTrack=cvCreateSeq(0, sizeof(CvSeq), sizeof(TRACK), storage);
	trackSet.head=NULL;
	trackSet.tail=NULL;
	trackSet.current=NULL;

	radii_hist_image = cvCreateImage(cvSize(320,200), 8, 1);
	dist_hist_image = cvCreateImage(cvSize(320,200), 8, 1);
	vel_hist_image = cvCreateImage(cvSize(320,200), 8, 1);
	
	cvNamedWindow("Image", 0);
	cvResizeWindow("Image", 800, 800);
  
  cvNamedWindow("Radii", 0);
  cvNamedWindow("Distance", 0);
  cvNamedWindow("Velocity", 0);
	
	for(i=0; i<(nTotalFrame-1)/TIFF_BLOCK_SIZE+1 ; i++)
	{
		if(i==(nTotalFrame-1)/TIFF_BLOCK_SIZE)
			nBlockFrame=(nTotalFrame-1)%TIFF_BLOCK_SIZE+1;
		else nBlockFrame=TIFF_BLOCK_SIZE;
		
//		printf("Reading Images...\n");
		readTIFF(fpImage, &idxFrame, nBlockFrame, &imgOrigin);
		
//		printf("Filtering Images...\n");
		imgBackground=cvCreateImage(cvSize(1004,1002), IPL_DEPTH_32F, 1);
		getBackground(imgOrigin, nBlockFrame, &imgBackground);
		getForeground(imgOrigin, nBlockFrame, &imgBackground);
  	cvShowImage("Image", imgBackground);
		getParameters(imgOrigin, nBlockFrame, &dThreshold, &dDiameter, &nJitter);

//		printf("Geting Positions...\n");
		position=malloc(nBlockFrame*sizeof(CvSeq*));
		getPosition(imgOrigin, nBlockFrame, dThreshold, dDiameter, nJitter, position);
		getPosStat(nBlockFrame, position, histRadii, histDist);

//		printf("Loading Tracks...\n");
		getTrack(position, nBlockFrame, &trackSet, currentTrack, storage);

/*		printf("Rendering Frames...\n");
		for(j=0;j<nBlockFrame;j++)
		{
			CvBox2D box;
			
			IplImage *buffer = cvCreateImage(cvSize(1004,1002), IPL_DEPTH_32F, 1);
			while((*(position+j))->total)
			{
				cvSeqPop(*(position+j), &box);
				cvEllipse(buffer, cvPointFrom32f(box.center), 
					cvSize(cvRound(box.size.width*0.5), cvRound(box.size.height*0.5)), 
					-box.angle, 0, 360, CV_RGB(255,255,255), 1, CV_AA, 0);
			}
			cvShowImage("Image", buffer);
			cvWaitKey(100);
		}
*/
		free(position);
		cvReleaseImage(&imgBackground);
		releaseTIFF(&imgOrigin, nBlockFrame);
	}

//	printf("Creating Histograms...\n");
	statTrack(&trackSet, histVel);

	getHist(&dist_hist_image, histDist, 2000);
	getHist(&radii_hist_image, histRadii, 2000);
	getHist(&vel_hist_image, histVel, 2000);
	
	cvShowImage("Distance", dist_hist_image);
	cvShowImage("Radii", radii_hist_image);
	cvShowImage("Velocity", vel_hist_image);
	
//	printf("Rendering Tracks...\n");
	renderTrack(&trackSet);
//	selectTrack(&trackSet);
//	renderTrack(&trackSet);
//	selectTrack(&trackSet);
	
//	printf("Closing...\n");
	closeTIFF(fpImage);
	return 0;
}
int main(int argc, char **argv)
{
	int radiiRange[2]={0,2000};
	loop(argv);
}
