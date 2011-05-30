/*
MOVIT: Analyse Particles' Motion Profile in Electrophoresis. 
*/
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#include "track.h"
#include <stdio.h>
#include <gtk/gtk.h>

#define TIFF_BLOCK_SIZE 100
#define STRIDE 16

typedef struct 
{
  char filename[200];
}SYSTEM;

int loop(char * filename)
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
  float distVel[1024/STRIDE][3]={0.0f};

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

  FILE * fp_out=NULL;
	
	if(!openTIFF(filename, &fpImage, &nTotalFrame, &idxFrame))
		exit(1);

  if (strstr (filename, ".tif") == filename + strlen (filename) - 4 )
  {
    filename[strlen(filename)-2]='x';
    filename[strlen(filename)-1]='t';
  }
  else
  {
    strcat(filename, ".txt");
  }
  
  fp_out = fopen(filename,"w");
//	printf("%d frames found in %s\n", nTotalFrame, filename[1]);							//debug
	
	storage=cvCreateMemStorage(0);
	currentTrack=cvCreateSeq(0, sizeof(CvSeq), sizeof(TRACK), storage);
	trackSet.head=NULL;
	trackSet.tail=NULL;
	trackSet.current=NULL;

//	radii_hist_image = cvCreateImage(cvSize(320,200), 8, 1);
//	dist_hist_image = cvCreateImage(cvSize(320,200), 8, 1);
//	vel_hist_image = cvCreateImage(cvSize(320,200), 8, 1);
	
//	cvNamedWindow("Image", 0);
//	cvResizeWindow("Image", 800, 800);
  
//  cvNamedWindow("Radii", 0);
//  cvNamedWindow("Distance", 0);
//  cvNamedWindow("Velocity", 0);
	
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
		getParameters(imgOrigin, nBlockFrame, &dThreshold, &dDiameter, &nJitter);

//		printf("Geting Positions...\n");
		position=malloc(nBlockFrame*sizeof(CvSeq*));
		getPosition(imgOrigin, nBlockFrame, dThreshold, dDiameter, nJitter, position);
		getPosStat(nBlockFrame, position, histRadii, histDist);

//		printf("Loading Tracks...\n");
		getTrack(position, nBlockFrame, &trackSet, currentTrack, storage);
    selectTrack(&trackSet);
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
  statTrack(&trackSet, histVel, 4.0f, distVel, fp_out);

//	getHist(&dist_hist_image, histDist, 2000);
//	getHist(&radii_hist_image, histRadii, 2000);
//	getHist(&vel_hist_image, histVel, 2000);
	
//	cvShowImage("Distance", dist_hist_image);
//	cvShowImage("Radii", radii_hist_image);
//	cvShowImage("Velocity", vel_hist_image);
	
//	printf("Rendering Tracks...\n");
//	renderTrack(&trackSet);
//	selectTrack(&trackSet);
//	renderTrack(&trackSet);
//	selectTrack(&trackSet);
	
//	printf("Closing...\n");
  fclose(fp_out);
	closeTIFF(fpImage);
	return 0;
}

static void open_file( GtkWidget *widget,
                         SYSTEM* system )
{
    GtkWidget *dialog;
    char *filename;

    dialog = gtk_file_chooser_dialog_new ("Open File",
                      NULL,
                      GTK_FILE_CHOOSER_ACTION_OPEN,
                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                      NULL);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    }
    g_print("%s\n",filename);
    strcpy (system->filename, filename);
    gtk_widget_destroy (dialog);
}

static void run( GtkWidget *widget,
                         SYSTEM* system )
{
  loop(system->filename);
}

int main(int argc, char **argv)
{
    SYSTEM system;
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *btn_open;
    GtkWidget *btn_run;

    GtkWidget *pbar;

    char **filename;
    
    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
    gtk_window_set_title (GTK_WINDOW (window), "MOVIT");
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);

    box = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), box);
    
    pbar = gtk_progress_bar_new ();
    gtk_box_pack_start (GTK_BOX(box), pbar, TRUE, TRUE, 0);
    gtk_widget_show (pbar);

    btn_open = gtk_button_new_with_label ("Open File");
    g_signal_connect (btn_open, "clicked", G_CALLBACK (open_file), (gpointer)&system);
    gtk_box_pack_start (GTK_BOX(box), btn_open, TRUE, TRUE, 0);
    gtk_widget_show (btn_open);

    btn_run = gtk_button_new_with_label ("Run");
    g_signal_connect (btn_run, "clicked", G_CALLBACK (run), (gpointer)&system);
    gtk_box_pack_start (GTK_BOX(box), btn_run, TRUE, TRUE, 0);
    gtk_widget_show (btn_run);

    gtk_widget_show (box);
    gtk_widget_show (window);
    
    gtk_main ();
    
  return 0;
}
