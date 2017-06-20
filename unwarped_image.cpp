//********************************************************************
// File name  : UBWARPED_IMAGE.CPP
// Designer : Shang-Yen Su @ Apr. 26, 2017
// Description:
// Unwarp segmented strip images into sa set of perspective images.
//********************************************************************

//#include <opencv2/opencv.hpp>
#include "header.h"

#ifdef DEBUG_UNWARP
FILE *fDebugUnwarp;
#endif


//===========================================
//Name:	unwraped_Image
//Input:	Mat maSegPanorama[]	==>Mat array of segmented images
//			Mat maUnwPanorama[]	==>Mat array of unwarped images
//			double daRadius	==>Radius of panorama image
//			unsigned int uaWidth	==>Width of panorama image
//			unsigned int uaHeight	==>Height of panorama image
//Output:	bool => Unwarping success or not
//Goal:
//Unwarp segmented images.
// Note:
// This is a working version!!
//===========================================
bool unwarped_Image(cv::Mat maSegPanorama[],cv::Mat maUnwPanorama[], double daRadius, unsigned int uiWidth, unsigned int uiHeight, unsigned int uaHeight, std::string alph){
    //Compute the strip width in pixels first
    int utStipWidth = uiWidth/SLIDES;
    //Compute the angle range of a segment
    double itSegSize = 360.0/SLIDES;
    //Compute the central angle of a pixel
    double dtPixelCA = itSegSize / utStipWidth;
    //Width of unwarpped perspective image
    int itUnwarpWidth = (int)(2 * daRadius * tan(itSegSize/2 * PI / 180.0));
    //Height of centraled column at the unwarpped perspective image
    int itCCHeight = (int)(2 * daRadius * tan(((uaHeight/2)*dtPixelCA - (dtPixelCA/2.0))* PI / 180.0));
    //Height of edge of the unwarpped perspective image
    //float Beta = (itSegSize/2 - dtPixelCA/2) * PI / 180.0;
    ///float value = cos(Beta);
    int itUnwarpHeight = itCCHeight/cos((itSegSize/2 - dtPixelCA/2) * PI / 180.0);
    //Check if uaHeight is even number or odd number?
    if((uaHeight%2)>0) itUnwarpHeight++;
    // Set a temporary unwarpped images storage area
    cv::Mat mtUnwPanorama;
    
#ifdef DEBUG01
    printf("Unwarpped width = %d & height = %d\n", itUnwarpWidth, itUnwarpHeight);
#endif

    
#ifdef DEBUG_UNWARP
        fDebugUnwarp = fopen("Debug/DebugUnwarp.txt", "w");
#endif
    
    // Unwarp segmented image into a perspective image
    // maSegPanorama[] contain slides * 2 strip images of the panorama.
    for(int i = 0 ; i < SLIDES*2 ; i ++){
        //Allocate memory to save unwarpped perspective image. Please refer to document on how to compute the size
        mtUnwPanorama = cv::Mat(itUnwarpHeight, itUnwarpWidth, CV_32SC1 );
        mtUnwPanorama = 500;//Initialize the memory
        for(int y = 0 ; y < itUnwarpHeight; y++)
            for(int x = 0 ; x < itUnwarpWidth; x++)
                mtUnwPanorama.at<char>(y, x) = 0;


#ifdef DEBUG_UNWARP
    if(i==0) fprintf(fDebugUnwarp, "------------ Unwarp segment %d ------------ \n", i);
#endif

        //Step 1: Unwarpping segmented image in top to down, left to right fashion
        for(int y = 0 ; y < uaHeight; y++)
        {// From top to down
            for(int x = 0 ; x < utStipWidth; x++)
            {//From left to right
                // Compute upwarpped U axis in the perspective image
                double dtBeta;
                int utUtemp, utUnwarpU;
                dtBeta = (dtPixelCA * x + (dtPixelCA/2.0)) - (itSegSize/2);
                utUtemp = (int)(daRadius * tan(dtBeta * PI / 180.0));
                utUnwarpU = (int)itUnwarpWidth/2 + utUtemp;
                
                // Compute upwarpped V axis in the perspective image
                int itHalfHeight = uaHeight/2;//Compute half height in pixel of the panorama
                int itVtemp, utUnwarpV;
                double dtAlpha, dtV;
                dtAlpha = ((y - itHalfHeight) + (dtPixelCA/2.0)) / uiHeight * PI;
                dtV = daRadius * tan(dtAlpha);
                itVtemp = (int)(dtV/cos(dtBeta * PI / 180.0));
                utUnwarpV = (int)(itUnwarpHeight/2) + itVtemp;

                // Now, copy the value of pixel in the strip image to the perspepective image
                // Before copy pixel value, since perspective image is expanded from strip image
                // The expension width has to added
                mtUnwPanorama.at<char>(utUnwarpV, utUnwarpU) = maSegPanorama[i].at<char>(y, x);
                
#ifdef DEBUG_UNWARP
    if(i==0)
        fprintf(fDebugUnwarp, "utUnwarpV = %d,\t utUnwarpU = %d,\t y = %d,\t x = %d\n", utUnwarpV, utUnwarpU, y, x);
#endif
            }
        }
#ifdef DEBUG_UNWARP
        imwrite("UNWARPED_IMAGE01/UNWARPED_" + alph + "_" + std::to_string(i) + ".bmp", mtUnwPanorama);
#endif

        // Step 2: Trim down upper-half and lower-half of the perspective image
        int start = (itUnwarpHeight - itCCHeight)/2;
        //Allocate memory to save unwarpped perspective image.
        maUnwPanorama[i] = cv::Mat(itCCHeight, itUnwarpWidth, CV_8UC1 );
        //Unwarpping in top to down, left to right fashion
        for(int y = 0 ; y < itCCHeight; y++)
            for(int x = 0 ; x < itUnwarpWidth; x++)
                maUnwPanorama[i].at<char>(y, x) = mtUnwPanorama.at<char>(y+start, x);

        
        //Step 3: Image Compensation
        // Allocate a matrix to save pixels' value with center as the pixle to be compensated
        // C matrix is sized of row by column
        int mat[3][3];
        for(int k = 0; k<2; k++ )// The image compensation must to twice to reduce noise
            for(int v = 1; v < itCCHeight - 1; v++){
                for(int u = 1; u < itUnwarpWidth-1 ; u++){
                    mat[0][0] = maUnwPanorama[i].at<uchar>(v-1, u-1);
                    mat[0][1] = maUnwPanorama[i].at<uchar>(v-1, u);
                    mat[0][2] = maUnwPanorama[i].at<uchar>(v-1, u+1);
                    mat[1][0] = maUnwPanorama[i].at<uchar>(v,   u-1);
                    mat[1][1] = maUnwPanorama[i].at<uchar>(v,   u);
                    mat[1][2] = maUnwPanorama[i].at<uchar>(v,   u+1);
                    mat[2][0] = maUnwPanorama[i].at<uchar>(v+1, u-1);
                    mat[2][1] = maUnwPanorama[i].at<uchar>(v+1, u);
                    mat[2][2] = maUnwPanorama[i].at<uchar>(v+1, u+1);
                    if(mat[1][1] == 0){//Empty cell is found. Need to compute its value.
                        if(mat[0][0]>0 || mat[0][1]>0 || mat[0][2]>0){// Any pixel at the top row is NOT empty
                            if(mat[2][0]>0 || mat[2][1]>0 || mat[2][2]>0){// Compensation is required
                                int itPCount = 0;
                                int itPValue = 0;
                                for(int i = 0; i < 3; i++)
                                    for(int j = 0; j < 3; j++){
                                        if(mat[i][j] > 0){
                                            itPCount ++;
                                            itPValue = itPValue + mat[i][j];
                                        }
                                    }
                                maUnwPanorama[i].at<uchar>(v, u) = (int)(itPValue/itPCount);
                            }
                        
                        }
                    }// else if non empty cell found, do nothing!
                }
            }
   
		#ifdef DEBUG01
			imwrite("UNWARPED_IMAGE02/UNWARPED_" + alph + "_" + std::to_string(i) + ".bmp", maUnwPanorama[i]);
		#endif
	}
    
#ifdef DEBUG_UNWARP
    fclose(fDebugUnwarp);
#endif

	return true;
}
