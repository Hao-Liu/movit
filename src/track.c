#include <cv.h>
#include <stdio.h>
#include "movit.h"

#include "track.h"
int cmpfunc( const void* _a, const void* _b, void* userdata)
{
	int *a = (int*)_a;
	int *b = (int*)_b;
	
	return (*a)>(*b)?-1:((*a)<(*b)?1:0);
}

int associate(TRACKSET *trackSet, CvSeq *curTrack, CvSeq *pos, CvSeq **tangle, CvSeq **emerge, CvMemStorage *storage)
{
	int i=0;
	int j=0;
	int threshold=2000;
	CvMat *pass=NULL;
	
	pass = cvCreateMat(curTrack->total, pos->total, CV_32SC1);
//	printf("%d %d\n", curTrack->total, pos->total);
	for(i=0;i<curTrack->total;i++)
	{
		for(j=0;j<pos->total;j++)
		{
			TRACK *cur=(TRACK *)cvGetSeqElem(curTrack, i);
			CvSeq *track=NULL;
			track=(CvSeq *)cur->track;
			TRACKITEM *item=(TRACKITEM *)cvGetSeqElem(track, cur->index);
			
			POSITION *posi=(POSITION *)cvGetSeqElem(pos, j);

			double x=posi->x;
			double y=posi->y;
			double vx=x-item->x;
			double vy=y-item->y;
			double ax=vx-item->vx;
			double ay=vy-item->vy;
			
			double dx=x-item->x_pre;
			double dy=y-item->y_pre;
			double dvx=vx-item->vx_pre;
			double dvy=vy-item->vy_pre;
			double dax=ax-item->ax;
			double day=ay-item->ay;
//			printf("%5.1lf %5.1lf %5.1lf %5.1lf %5.1lf %5.1lf\n",
//				item->x, item->y, item->vx, item->vy, item->ax, item->ay);
			
//			printf("%5.1lf %5.1lf %5.1lf %5.1lf %5.1lf %5.1lf\n",
//				x, y, vx, vy, ax, ay);

//			printf(j==pos->total-1?"%7.0lf\n":"%7.0lf ",(dx*dx+dy*dy+dvx*dvx+dvy*dvy+dax*dax+day*day));
//			printf("Difference: %5.1lf %5.1lf %5.1lf %5.1lf %5.1lf %5.1lf %5.1lf\n",
//				dx, dy, dvx, dvy, dax, day, (dx*dx+dy*dy+dvx*dvx+dvy*dvy+dax*dax+day*day));

			cvSet2D(pass, i, j, cvRealScalar((dx*dx+dy*dy+dvx*dvx+dvy*dvy+dax*dax+day*day)));
		}
	}
	
	int rows = curTrack->total;
	int cols = pos->total;
	

	CvSeq *c=cvCloneSeq(curTrack, NULL);
	CvSeq *p=cvCloneSeq(pos, NULL);
	
	CvSeq *c_del=cvCreateSeq(0, sizeof(CvSeq), sizeof(int), storage);
	CvSeq *p_del=cvCreateSeq(0, sizeof(CvSeq), sizeof(int), storage);
	
	while(rows && cols)
	{
		double minVal=0.0;
		double maxVal=0.0;
		CvPoint minLoc;
		TRACK *current=NULL;
		CvSeq *track=NULL;
		
//		printf("rows:%d cols:%d\n", rows, cols);
		cvMinMaxLoc(pass, &minVal, &maxVal, &minLoc, NULL, NULL);
		if(minVal>threshold)
			break;
		for(i=0;i<curTrack->total;i++)
		{
//			printf("%d %d\n", i, minLoc.x);
			cvSet2D(pass, i, minLoc.x, cvRealScalar(maxVal));
		}
		for(i=0;i<pos->total;i++)
		{
//			printf("%d %d\n", minLoc.y, i);
			cvSet2D(pass, minLoc.y, i, cvRealScalar(maxVal));
		}

		current = (TRACK *)cvGetSeqElem(curTrack, minLoc.y);
		track=(CvSeq *)(current->track);
		TRACKITEM *item=(TRACKITEM *)cvGetSeqElem(track, current->index);

		POSITION *posi=(POSITION *)cvGetSeqElem(pos, minLoc.x);
		
		CvMat* measurement = cvCreateMat( 2, 1, CV_32FC1 );
		cvSet1D(measurement, 0, cvRealScalar(posi->x));
		cvSet1D(measurement, 1, cvRealScalar(posi->y));
		cvKalmanCorrect(item->kalman, measurement);
		cvReleaseMat(&measurement);

		item->disappear=0;
		item->x = posi->x;
		item->y = posi->y;
		item->vx= item->vx_pre;
		item->vy= item->vy_pre;
		item->ax= item->ax_pre;
		item->ay= item->ay_pre;
		item->angle = posi->angle;
		item->width = posi->width;
		item->height= posi->height;
		
		cvSeqPush(c_del, &(minLoc.y));
		cvSeqPush(p_del, &(minLoc.x));
		
		rows--;
		cols--;
	}
/*
	for(i=0;i<c_del->total;i++)
	{
		printf("%d ", *(cvGetSeqElem(c_del, i)));
	}
	printf("\n");
	for(i=0;i<p_del->total;i++)
	{
		printf("%d ", *(cvGetSeqElem(p_del, i)));
	}
	printf("\n");
*/	
	cvSeqSort(c_del, cmpfunc, NULL);
	cvSeqSort(p_del, cmpfunc, NULL);
/*
	for(i=0;i<c_del->total;i++)
	{
		printf("%d ", *(cvGetSeqElem(c_del, i)));
	}
	printf("\n");
	for(i=0;i<p_del->total;i++)
	{
		printf("%d ", *(cvGetSeqElem(p_del, i)));
	}
	printf("\n");
*/	
//	printf("%d Associated Track(s)   %d Associated Point(s)\n", c_del->total, p_del->total);
	for(i=0;i<c_del->total;i++)
	{
//		printf("Removing Associated track %d, \n", *(cvGetSeqElem(c_del, i)));
		cvSeqRemove(c, *(cvGetSeqElem(c_del, i)));
	}

	for(i=0;i<p_del->total;i++)
	{
//		printf("Removing Associated point %d, \n", *(cvGetSeqElem(p_del, i)));
		cvSeqRemove(p, *(cvGetSeqElem(p_del, i)));
	}
//	printf("%d Tangling Track(s)   %d New Point(s)\n", c->total, p->total);
	
	(*tangle)=c;
	(*emerge)=p;
	cvReleaseMat(&pass);
}

int predict(TRACKSET *trackSet, CvSeq *curTrack, CvMemStorage *storage)
{
	int i=0;
	int j=0;
	int k=0;
	TRACK *cur=NULL;
	CvSeq *track=NULL;
	
	for(i=0;i<curTrack->total;i++)
	{
		cur=(TRACK *)cvGetSeqElem(curTrack, i);
		track=(CvSeq *)(cur->track);
		TRACKITEM *prevItem=(TRACKITEM *)cvGetSeqElem(track, cur->index);
		cvKalmanPredict(prevItem->kalman, NULL);

		TRACKITEM item;
		item.index=prevItem->index+1;
		item.x_pre  = prevItem->kalman->state_pre->data.fl[0];
		item.y_pre  = prevItem->kalman->state_pre->data.fl[1];
		item.vx_pre = prevItem->kalman->state_pre->data.fl[2];
		item.vy_pre = prevItem->kalman->state_pre->data.fl[3];
		item.ax_pre = prevItem->kalman->state_pre->data.fl[4];
		item.ay_pre = prevItem->kalman->state_pre->data.fl[5];
//		printf("Predicted : %5.1lf %5.1lf %5.1lf %5.1lf %5.1lf %5.1lf \n", 
//			item.x_pre,item.y_pre,item.vx_pre,item.vy_pre,item.ax_pre,item.ay_pre);
//		printf("%d %d\n", prevItem->kalman->gain->rows,prevItem->kalman->gain->cols);
//		for(j=0;j<prevItem->kalman->gain->rows;j++)
//		{
//			for(k=0;k<prevItem->kalman->gain->cols;k++)
//			{
//				printf("%f ", prevItem->kalman->gain->data.fl[j*prevItem->kalman->gain->cols+k]);
//			}
//			printf("\n");
//		}
		item.disappear = prevItem->disappear;
		item.x = prevItem->x;
		item.y = prevItem->y;
		item.vx= prevItem->vx;
		item.vy= prevItem->vy;
		item.ax= prevItem->ax;
		item.ay= prevItem->ay;
		item.angle = 0.0;
		item.width = 0.0;
		item.height= 0.0;
		item.kalman = prevItem->kalman;
		cvSeqPush(track, &item);
		(cur->index)++;
	}
	return 1;
}

int create(TRACKSET *trackSet, CvSeq *curTrack, CvSeq *pos, CvMemStorage *storage)
{
	int i=0;
	const float A[]={ 1.0, 0.0, 1.0, 0.0, 0.5, 0.0, 
										0.0, 1.0, 0.0, 1.0, 0.0, 0.5, 
										0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 
										0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 
										0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 
										0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
										
	for(i=0;i<pos->total;i++)
	{
		TRACKITEM item;
		CvSeq *track;
		CvKalman *kalman;
		TRACK cur;
		POSITION posi;
		
		memcpy(&posi, cvGetSeqElem(pos, i), sizeof(POSITION));		

		track=cvCreateSeq(0, sizeof(CvSeq), sizeof(TRACKITEM), storage);
		if(trackSet->head==NULL)
		{
			trackSet->head=track;
			trackSet->tail=track;
			track->h_prev=NULL;
			track->h_next=NULL;
		}
		else
		{
			track->h_prev=trackSet->tail;
			trackSet->tail->h_next=track;
			trackSet->tail=track;
			track->h_next=NULL;
		}
		
		kalman=cvCreateKalman(6, 2, 0);
		memcpy(kalman->transition_matrix->data.fl, A, sizeof(A));
		cvSetIdentity( kalman->measurement_matrix, cvRealScalar(1) );
		cvSetIdentity( kalman->measurement_noise_cov, cvRealScalar(1e-1) );
		cvSetIdentity( kalman->error_cov_post, cvRealScalar(1));
		cvSetIdentity( kalman->process_noise_cov, cvRealScalar(1e-5) );
		cvZero(kalman->state_post);
		cvSet1D(kalman->state_post,0, cvRealScalar(posi.x));
		cvSet1D(kalman->state_post,1, cvRealScalar(posi.y));


		item.index=0;
		item.disappear=0;
		item.x = posi.x;
		item.y = posi.y;
		item.vx= 0.0;
		item.vy= 0.0;
		item.ax= 0.0;
		item.ay= 0.0;
		item.x_pre = 0.0;
		item.y_pre = 0.0;
		item.vx_pre= 0.0;
		item.vy_pre= 0.0;
		item.ax_pre= 0.0;
		item.ay_pre= 0.0;
		item.angle = posi.angle;
		item.width = posi.width;
		item.height= posi.height;
		item.kalman = kalman;
		cvSeqPush(track, &item);

		cur.track=track;
		cur.index=0;
		cvSeqPush(curTrack, &cur);
	}
}

int searchfunc(const void* _a, const void* _b, void* userdata)
{
	TRACK *curTrack = (TRACK *)_a;
	TRACK *track = (TRACK *)_b;
	
//	printf("targetTrack: %d  iterateTrack: %d\n", curTrack->track, track->track);
	return curTrack->track==track->track?0:1;
}


int terminate(TRACKSET *trackSet, CvSeq *target, CvSeq *curTrack, CvMemStorage *storage)
{
	int idx=0;
	
	CvSeq *targetTrack=cvCloneSeq(target, NULL);
	
	while(targetTrack->total)
	{
		TRACK *cur = (TRACK *)cvGetSeqElem(curTrack, 0);
		CvSeq *track=(CvSeq *)(cur->track);
		TRACKITEM *item=(TRACKITEM *)cvGetSeqElem(track, cur->index);
	
//		printf("Tangling Location : (%4.0f,%4.0f ) Disappeared %d time(s)\n", item->x_pre, item->y_pre,  item->disappear);

		if(item->x_pre<0.0 || item->x_pre> 1004.0 || item->y_pre<0.0 || item->y_pre>1002.0 || item->disappear>4 )
		{
			cvSeqSearch(curTrack, cur, searchfunc, 0, &idx, NULL);
//			printf("Terminating: %d (status:%d)\n", idx, status);
			cvSeqRemove(curTrack, idx);
			cvSeqRemove(targetTrack, 0);
		}
		else
		{
			CvMat* measurement = cvCreateMat( 2, 1, CV_32FC1 );
			cvSet1D(measurement, 0, cvRealScalar(item->x_pre));
			cvSet1D(measurement, 1, cvRealScalar(item->y_pre));
			cvKalmanCorrect(item->kalman, measurement);
			cvReleaseMat(&measurement);

			item->disappear++;
			item->x = item->x_pre;
			item->y = item->y_pre;
			item->vx= item->vx_pre;
			item->vy= item->vy_pre;
			item->ax= item->ax_pre;
			item->ay= item->ay_pre;
			item->angle = 0.0;
			item->width = 0.0;
			item->height= 0.0;
			cvSeqRemove(targetTrack, 0);
		}
	}
	return 1;
}

int dataAssociate(TRACKSET *trackSet, CvSeq *curTrack, CvSeq *pos, CvMemStorage *storage)
{
	CvSeq *tangle;
	CvSeq *emerge;
	
	if(curTrack->total==0 && pos->total==0)
	{
		return 1;
	}
	if(curTrack->total==0)
	{
		create(trackSet, curTrack, pos, storage);
		return 1;
	}
	if(pos->total==0)
	{
		predict(trackSet, curTrack, storage);
		terminate(trackSet, curTrack, curTrack, storage);
	}
	else
	{
		predict(trackSet, curTrack, storage);
		associate(trackSet, curTrack, pos, &tangle, &emerge, storage);
		terminate(trackSet, tangle, curTrack, storage);
		create(trackSet, curTrack, emerge, storage);
	}
	return 1;
}

int	getTrack(CvSeq **pos, int n, TRACKSET *trackSet, CvSeq *curTrack, CvMemStorage *storage)
{
	int i=0;
	int j=0;
	for(i=0;i<n;i++)
	{
//		printf("Frame: %d\n", i);
		dataAssociate(trackSet, curTrack, *(pos+i), storage);
/*		
		IplImage *buffer = cvCreateImage(cvSize(1004,1002), IPL_DEPTH_32F, 3);
		for(j=0;j<curTrack->total;j++)
		{
			TRACK *cur = (TRACK *)cvGetSeqElem(curTrack, j);
			CvSeq *track=(CvSeq *)(cur->track);
			TRACKITEM *item=(TRACKITEM *)cvGetSeqElem(track, cur->index);

			cvEllipse(buffer, cvPoint(cvRound(item->x), cvRound(item->y)), 
				cvSize(cvRound(item->width*0.5), cvRound(item->height*0.5)), 
				-item->angle, 0, 360, CV_RGB(((long)(track)/1024%2+1)*128,(long)(track)/2048%2*128,(long)(track)/4096%2*128), 1, CV_AA, 0);
//			printf("x:%5.1f y:%5.1f w:%5.1f h:%5.1f ratio:%1.2f\n", item->x, item->y, item->width, item->height, item->height/item->width);
		}
			cvShowImage("Image", buffer);
			cvWaitKey(50);
			cvZero(buffer);
		cvReleaseImage(&buffer);
*/
	}
	return 1;
}
int renderTrack(TRACKSET *trackSet)
{
	int i=0;
	
	IplImage *buffer = cvCreateImage(cvSize(1004,1002), IPL_DEPTH_32F, 3);
	CvSeq *track = trackSet->head;
	while(track->h_next)
	{
//		printf("Rendering track %10ld with %d data!\n", (long)track, track->total);
		for(i=0;i<track->total;i++)
		{
			TRACKITEM *item = (TRACKITEM *)cvGetSeqElem(track, i);
			if(item->width==0.0)
			{
//				cvRectangle(buffer, 
//					cvPoint(cvRound(item->x-20.0), cvRound(item->y-20.0)), 
//					cvPoint(cvRound(item->x+20.0), cvRound(item->y+20.0)),
//					CV_RGB(((long)(track)/1024%2+1)*128,(long)(track)/2048%2*128,(long)(track)/4096%2*128), 1, CV_AA, 0);
			}
			cvEllipse(buffer, cvPoint(cvRound(item->x), cvRound(item->y)), 
				cvSize(cvRound(item->width*0.5), cvRound(item->height*0.5)), 
				-item->angle, 0, 360, CV_RGB(((long)(track)%7+2)*32,(long)(track)/2048%2*128,(long)(track)/4096%2*128), 1, CV_AA, 0);
//			printf("x:%5.1f y:%5.1f w:%5.1f h:%5.1f ratio:%1.2f\n", item->x, item->y, item->width, item->height, item->height/item->width);
		}
		track=track->h_next;
	}
			cvShowImage("Image", buffer);
			cvWaitKey(0);
			cvZero(buffer);

}
int selectTrack(TRACKSET *trackSet)
{
	int i=0;
	double sumx=0;
	double sumy=0;
	double sumxy=0;
	double sumx2=0;
	double sumy2=0;
	double rsumx=0;
	double rsumy=0;
	double rsumxy=0;
	double rsumx2=0;
	double rsumy2=0;
	int realtotal=0;
	double r=0;
	double rr=0;
	CvSeq *track = trackSet->head;
	while(track->h_next)
	{
		sumx=0;
		sumy=0;
		sumxy=0;
		sumx2=0;
		sumy2=0;

		rsumx=0;
		rsumy=0;
		rsumxy=0;
		rsumx2=0;
		rsumy2=0;

		realtotal=0;
		for(i=0;i<track->total;i++)
		{
			
			TRACKITEM *item = (TRACKITEM *)cvGetSeqElem(track, i);
			if(item->width==0)
			{
			}
			else
			{
				sumx+=item->x;
				sumy+=item->y;
				sumxy+=item->x*item->y;
				sumx2+=item->x*item->x;
				sumy2+=item->y*item->y;

				rsumx+=(double)(i);
				rsumy+=item->height;
				rsumxy+=(double)(i)*item->height;
				rsumx2+=(double)(i)*(double)(i);
				rsumy2+=item->height*item->height;

				realtotal++;
			}
		}
		r=(realtotal*sumxy-sumx*sumy)/
			sqrt((realtotal*sumx2-sumx*sumx)*(realtotal*sumy2-sumy*sumy));
		rr=(realtotal*rsumxy-rsumx*rsumy)/
			sqrt((realtotal*rsumx2-rsumx*rsumx)*(realtotal*rsumy2-rsumy*rsumy));

		if((double)realtotal/(double)(track->total)>0.7 && realtotal>4 /*&& rr > 0.5*/)
		{
//			printf("*%d %d %f %f %f\n", realtotal, track->total, (double)realtotal/(double)(track->total), r, rr);
		}
		else
		{
			if(track->h_prev)
			{
				track->h_prev->h_next=track->h_next;
				track->h_next->h_prev=track->h_prev;
			}
			else
			{
				trackSet->head=track->h_next;
				track->h_next->h_prev=NULL;
			}
//			printf(" %d %d %f %f %f\n", realtotal, track->total, (double)realtotal/(double)(track->total), r, rr);
		}
		track=track->h_next;
	}
	return 1;
}

int statTrack(TRACKSET *trackSet, int *hist, float stride, float *dist)
{
	int i=0;
  stride = 16.0f;
	int pos;
	CvSeq *track = trackSet->head;
        printf("Width\tHeight\tPosX\tPosY\tVelX\tVelY\tTime\n");
	while(track->h_next)
	{
/*
	  TRACKITEM *item1 = (TRACKITEM *)cvGetSeqElem(track, 0);
	  TRACKITEM *item2 = (TRACKITEM *)cvGetSeqElem(track, track->total-3);
	  float dy = -(item2->y - item1->y)/(track->total-2);
		printf("%f\n",dy);
		if(dy>0.0)
		hist[cvFloor(dy)]++;
		if(dy<0.0)
		hist[cvFloor(-dy)]++;
*/
		for(i=0;i<track->total-1;i++)
		{
      TRACKITEM *item1 = (TRACKITEM *)cvGetSeqElem(track, i);
      TRACKITEM *item2 = (TRACKITEM *)cvGetSeqElem(track, i+1);
      if (item1->height>1e-5 && item2->height>1e-5)
      {
        float dx, dy, x, y, h, w, t;
        dy = fabsf(item2->y - item1->y);
        dx = fabsf(item2->x - item1->x);
        x = fabsf(item1->x + item2->x)/2.0f;
        y = fabsf(item1->y + item2->y)/2.0f;
        h = fabsf(item1->height + item2->height)/2.0f;
        w = fabsf(item1->width + item2->width)/2.0f;
	t = fabsf((float)item1->index + (float)item2->index)/2.0f*0.044f;
        printf("%e %e %e %e %e %e %e\n", w, h, x, y, dx, dy, t);
        hist[cvFloor(dy)]++;
        pos = (int)(x/stride);
        *(dist+3*pos+0)+=1.0f;
        *(dist+3*pos+1)+=dy;
        *(dist+3*pos+2)=*(dist+3*pos+1)/ *(dist+3*pos+0);
      }
		}

		track=track->h_next;
	}
  /*
  for(i=0; i<1024/(int)stride; i++)
  {
    printf("%d %f\n", (int)*(dist+3*i+0), *(dist+3*i+2));
  }*/
}
