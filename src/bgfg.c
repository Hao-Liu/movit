#include <cv.h>
#include "movit.h"

int getBackground(IplImage **img, int n, IplImage **bg)
{
	int i=0;
	
	for(i=0; i<n; i++)
	{
		cvAdd(*(img+i), *bg, *bg, NULL);
	}	
	cvScale(*bg, *bg, 1.0/(double)n, 0);
	return 1;
}

int getForeground(IplImage **img, int n, IplImage **bg)
{
	int i=0;

	for(i=0; i<n; i++)
	{
		cvDiv(*bg, *(img+i), *(img+i), 1.0);
		cvLog(*(img+i), *(img+i));
	}
	return 1;
}

int getParameters(IplImage **img, int n, double *thr, double *dia, int *jit)
{
	(*thr)=0.03;
	(*jit)=5;
	(*dia)=5.0;
	return 1;
}

int isIn(CvBox2D *b, CvBox2D *a)
{
	if( b->center.x > a->center.x-a->size.height/2 &&
			b->center.x < a->center.x+a->size.height/2 &&
			b->center.y > a->center.y-a->size.height/2 &&
			b->center.y < a->center.y+a->size.height/2 &&
			b->size.height < a->size.height
			)
	return 1;
	else
	return 0;
}

int getPosition(IplImage **img, int n, double thr, double dia, int jit, CvSeq **pos)
{
	int i=0;
	
	CvMemStorage* storage;
	CvSeq* contour;

	IplImage* buffer;

	
	for(i=0; i<n; i++)
	{

		cvSmooth(*(img+i), *(img+i), CV_GAUSSIAN, jit, jit, 0,0);
//		cvScale(*(img+i), *(img+i), 40.0, 0.0);
		cvThreshold(*(img+i), *(img+i), thr, 1.0, CV_THRESH_BINARY);
		storage = cvCreateMemStorage(0);
    contour = cvCreateSeq(CV_SEQ_ELTYPE_POINT, 
    					sizeof(CvSeq), sizeof(CvPoint) , storage);

    *(pos+i) = cvCreateSeq(0, sizeof(CvSeq), sizeof(POSITION) , storage);
		
		buffer = cvCreateImage(cvSize(1004,1002), IPL_DEPTH_8U, 1);
		cvScale(*(img+i), buffer, 256.0, 0.0);
    cvFindContours( buffer, storage, &contour, sizeof(CvContour),
                    CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0,0));
    for(;contour;contour = contour->h_next)
    {
        int count = contour->total; // This is number point in contour
        CvPoint center;
        CvSize size;
        CvBox2D box;
        POSITION posi;

        // Number point must be more than or equal to 6 (for cvFitEllipse_32f).
        if( count < 6 )
            continue;

        CvMat* points_f = cvCreateMat( 1, count, CV_32FC2 );
        CvMat points_i = cvMat( 1, count, CV_32SC2, points_f->data.ptr );
        cvCvtSeqToArray( contour, points_f->data.ptr, CV_WHOLE_SEQ );
        cvConvert( &points_i, points_f );

        // Fits ellipse to current contour.
        box = cvFitEllipse2( points_f );
				
				if( box.size.width != 0.0 &&	box.size.height != 0.0 &&
      		box.size.height > dia && 
					box.size.height/box.size.width<5.0)
				{	
				  posi.x = box.center.x;
				  posi.y = box.center.y;
				  posi.width = box.size.width;
				  posi.height = box.size.height;
				  posi.angle = box.angle;
				  posi.frame = i;
					cvSeqPush(*(pos+i), &posi);
				}
        cvReleaseMat(&points_f);
    }
    
    int j=0;
    int k=0;
    
    while(j!=(*(pos+i))->total)
    {
    	k=j+1;
    	while(k!=(*(pos+i))->total)
    	{
    		CvBox2D *box_a;
    		CvBox2D *box_b;
//	    		printf("%d %d %d\n", j, k, (*(pos+i))->total);
    		box_a = (CvBox2D *)cvGetSeqElem((*(pos+i)), j);
    		box_b = (CvBox2D *)cvGetSeqElem((*(pos+i)), k);
    		if(isIn(box_b,box_a))
    		{
    			cvSeqRemove((*(pos+i)), k);
    			
    		}
    		else if(isIn(box_a,box_b))
    		{
    			cvSeqRemove((*(pos+i)), j);
    			j--;
    			break;    			
    		}
    		else
    		{	
    			k++;
    		}
    	}
    	j++;
    }
	}
	cvReleaseImage(&buffer);
	return 1;
}

int getPosStat(int n, CvSeq **position, int *histRadii, int *histDist)
{
	int i=0;
	int j=0;
	int k=0;
	
	for(i=0;i<n;i++)
	{
		for(j=0;j<(*(position+i))->total;j++)
		{
			int minsq=10000000;
			
			POSITION *pos1;
			pos1 = (POSITION *)cvGetSeqElem((*(position+i)), j);
			(*(histRadii+cvFloor(pos1->height)))++;
			for(k=0;k<(*(position+i))->total;k++)
			{
				if(k!=j)
				{
				  POSITION *pos2;
					pos2 = (POSITION *)cvGetSeqElem((*(position+i)), k);
					int distsq = 	(pos2->x-pos1->x)*(pos2->x-pos1->x)+
												(pos2->y-pos1->y)*(pos2->y-pos1->y);
					if(distsq<minsq)
					{
						minsq=distsq;
					}
				}
			}
			if(minsq<10000000)
			{
//				printf(j==(*(position+i))->total-1?"%4f\n":"%4f ", sqrt((double)minsq));
				(*(histDist+cvFloor(sqrt((double)minsq))))++;
			}
		}
	}
	
	return 1;
}

int getHist(IplImage **hist_image, int *hist, int n)
{
	int i=0;
	int max_x=0;
	int max_y=0;
	int bin_w = 1;
	
	for(i=0;i<n;i++)
	{
		if(hist[i])
		max_x=i;
		if(hist[i]>max_y)
		max_y=hist[i];
	}
    cvSet( *hist_image, cvScalarAll(255), 0 );
    bin_w = cvRound((double)(*hist_image)->width/max_x);

	for(i=0;i<max_x;i++)
	{
//		printf("%d %d %d %d \n",i*bin_w, (*hist_image)->height, (i+1)*bin_w, (*hist_image)->height*(1.0 - (double)hist[i]/(double)max_y));
    cvRectangle( *hist_image, cvPoint(i*bin_w, (*hist_image)->height),
                 cvPoint((i+1)*bin_w, (*hist_image)->height*(1.0 - (double)hist[i]/(double)max_y)),
                 cvScalarAll(0), -1, 8, 0 );
	}
//	cvShowImage("histogram", *hist_image);
}
