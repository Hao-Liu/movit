#include <stdio.h>
#include <cv.h>
//#include <cxcore.h>
//#include <highgui.h>

int openTIFF(char *filename, FILE **fp, int *numFrame, int *first)
{
	unsigned int buffer=0;
	unsigned int i=0;
	unsigned int sNumEntry=0;

	(*fp)=fopen(filename, "rb");
	(*numFrame)=0;
	if(!(*fp))
	{
		printf("Target file doesn't exist!\n");
		return 0;
	}

	//Read and Identify Metadata	
	if(fread(&buffer, 4, 1, (*fp)))
	{
		if(buffer==2771273)
		{
			//printf("This is a little endian TIFF file!\n");
		}
		else if(buffer==704662861)
		{
			printf("This is a big endian TIFF file!\n");
			return 0;

		}
		else
		{
			printf("This is not a TIFF file!\n");
			return 0;
		}		
	}
	else
	{
		printf("File is too small!");
		return 0;
	}

	//Get Frame Number and First Index
	if(fread(&buffer, 4, 1, (*fp)))
	{
		if(buffer)
		{
			(*first)=buffer;
			while(!fseek((*fp), buffer, SEEK_SET))
			{
				sNumEntry=0;
				fread(&sNumEntry, 2, 1, (*fp));
				for(i=0; i<sNumEntry*3; i++)
				{
					fread(&buffer, 4, 1, (*fp));
				}
				(*numFrame)++;
				fread(&buffer, 4, 1, (*fp));
				if(!buffer) break;
			}
		}
		else
		{
			printf("No image in this file!");
			return 1;
		}

	}
	else
	{
		printf("File is too small!");
		return 1;
	}
	return 1;
}

int closeTIFF(FILE* fp)
{
	fclose(fp);
	fp=NULL;
	return 1;
}

int	readTIFF(FILE *fp,int *idx,int nFrame, IplImage ***img)
{
	unsigned int i=0;
	unsigned int j=0;
	unsigned int pStrip=0;
	unsigned int pData=0;
	unsigned int sNumEntry=0;
	unsigned int tag=0;
	unsigned int type=0;
	unsigned int nvalue=0;
	unsigned int offset=0;
	unsigned int height=0;
	unsigned int width=0;
	unsigned int nStrip=0;
	unsigned int nRow=0;
	
	IplImage *buffer=NULL;
	
	
	(*img)=malloc(sizeof(IplImage *)*nFrame);

	buffer=cvCreateImage(cvSize(1004,1002), IPL_DEPTH_16U, 1);

	for(i=0; i<nFrame; i++)
	{
		*((*img)+i)=cvCreateImage(cvSize(1004,1002), IPL_DEPTH_32F, 1);
		
		fseek(fp, (*idx), SEEK_SET);
		sNumEntry=0;
		fread(&sNumEntry, 2, 1, fp);
		
		for(j=0; j<sNumEntry; j++)
		{
			fread(&tag, 2, 1, fp);
			fread(&type, 2, 1, fp);
			fread(&nvalue, 4, 1, fp);
			fread(&offset, 4, 1, fp);
			if(tag==0x101)
			{
				height = offset;
			}
			if(tag==0x100)
			{
				width = offset;
			}
			if(tag==0x111)
			{
				nStrip = nvalue;
				pStrip = offset;
			}
			if(tag==0x116)
			{
				nRow = offset;
			}
		}
		fread(idx, 4, 1, fp);
		for(j=0; j<nStrip; j++)
		{
			fseek(fp, pStrip+j*4, SEEK_SET);
			fread(&pData, 4, 1, fp);
			fseek(fp, pData, SEEK_SET);
			
			fread(buffer->imageData+j*width*nRow*2, 
				(j==nStrip-1)?width*(height%nRow)*2:width*nRow*2, 1, fp);
		}
		cvScale(buffer, *((*img)+i), 1.0, 0.0);
	}
	
	cvReleaseImage(&buffer);
	return 1;
}

int releaseTIFF(IplImage ***img, int nFrame)
{
	unsigned int i=0;

	for(i=0; i<nFrame; i++)
	{
		cvReleaseImage((*img)+i);
	}
	free(*img);
	(*img)=NULL;
	return 1;
}
