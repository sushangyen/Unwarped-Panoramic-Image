//********************************************************************
// File name  : SEGMENT_IMAGE.CPP
// Designer : Shang-Yen Su
// Create date: Apr. 20, 2017
// Description:
// This file segments panorama images.
// Convention:
// Prefix: s = string; i = integer; u = unisgned integer; f = float
//         d = double; m = dynamic memory; a = argument; t = temp
//********************************************************************

//#include <opencv2/opencv.hpp>
#include "header.h"

//===========================================
//Name:	segment_Image
//Input: maInputPanorama => The panorama to be segmented
//		 maSegPanorama[] => The resulted strip images. The execution result will be returned from here.
//		 uaWidth	==> Width of panorama image
//		 uaHeight	==> Height of panorama image
//       iaShortage ==> Shortage of panorama's origin width in the multiple of 8.
//Output:	Success or not
//Description:
//Segment a panorama into 8 image slides.
//
// Notice:
// The argument, isShortage, notify if the origin wodth of the panorama is the multiple of 8?
// If not, we need to insert a column of zero pixels on rightmost edge of "iaShortage" strips.
//TODO: Any dummy check if this operation is success or not?
//TODO: Segmentation should do twice with 22.5 degree apart.
//NEED FIX:
// The panorama should segmented into two sets of strip images. Two sets are
//   is shift by 22.5 degree.
//===========================================
bool segment_Image(cv::Mat maInputPanorama, cv::Mat maSegPanorama[], unsigned int uaWidth, unsigned int uaHeight, int iaShortage, std::string alph){
    //If iaShortage > 0, the origin width of panorama is NOT the multiple of 8.
    // ==> Need to insert a column of dummy pixels on rightmost edge of "iaShortage" strips.
    //Segment a panorama into a set of image strip with 45 degree apart.
    int itShortage = iaShortage;
    unsigned int utStripWidth = uaWidth/SLIDES;
    unsigned int utPixelCount;
    
    //PSS A: The 1st segmentation is started from leftmost pixel to rightmost pixel in width of the panorama.
    utPixelCount = 0;// Reset the pixel counter
    for(int i = 0 ; i < SLIDES ; i ++){
        //Dynamic allocate memory to save segmented strip image
        maSegPanorama[i] = cv::Mat(uaHeight, utStripWidth, CV_8UC1 );
        // Check if the memory allocation success or not?
        if(maSegPanorama[i].rows == 0 || maSegPanorama[i].cols == 0){
            printf("Error!! Memory allocation failure in Segmentation!!!\n");
            return false;
        }

        if(itShortage > 0){
            //The origin width is NOT the multiple of 8. Need insert a column of zero pixels.
            for(int j = 0 ; j < uaHeight ; j ++){
                for(int k = 0 ; k < (utStripWidth-1) ; k ++)
                    maSegPanorama[i].at<uchar>(j, k) = maInputPanorama.at<uchar>(j, k + utPixelCount);
                //Insert a zero pixel on the rightmost edge of the strip image
                maSegPanorama[i].at<uchar>(j, utStripWidth-1) = 255;
//TODO: Need to check if the dummy pixel should 0 or 255?
            }
            itShortage --;
        }else{
            //The origin width is the multiple of 8.
            for(int j = 0 ; j < uaHeight ; j ++)
                for(int k = 0 ; k < utStripWidth ; k ++)
                    maSegPanorama[i].at<uchar>(j, k) = maInputPanorama.at<uchar>(j, k + utPixelCount);
        }
        //Update the width pixel counter
        utPixelCount = utPixelCount + utStripWidth;
#ifdef DEBUG01
        cv::imwrite("SEGMENT_IMAGE/SEGMENTED_" + alph + "_" + std::to_string(i) + ".bmp", maSegPanorama[i]);
#endif
    }
        
    //PSS B: The 2nd segmentation is started from hift half of utStripWidth to rightmost in width of the panorama.
    //Hence, need extra care to wrap up leftmost pixel with rightmost pixel. 
    utPixelCount = 0;// Reset the pixel counter
    for(int i = 0 ; i < SLIDES ; i ++){
        //Dynamic allocate memory to save segmented strip image
        maSegPanorama[SLIDES + i] = cv::Mat(uaHeight, utStripWidth, CV_8UC1 );
        // Check if the memory allocation success or not?
        if(maSegPanorama[SLIDES + i].rows == 0 || maSegPanorama[SLIDES + i].cols == 0){
            printf("Error!! Memory allocation failure in Segmentation!!!\n");
            return false;
        }
        if(itShortage > 0){
            //The origin width is NOT the multiple of 8. Need insert a column of zero pixels.
            if(i != (SLIDES-1)){//PSS B is started from shift half of utStripWidth to right on the panorama.
                for(int j = 0 ; j < uaHeight ; j ++){
                    for(int k = 0 ; k < (utStripWidth-1) ; k ++){
                        //PSS B shifted 22.5 degree, hence doesn`t start from zero.
                        maSegPanorama[SLIDES + i].at<uchar>(j, k) = maInputPanorama.at<uchar>(j, utStripWidth/2 + k + utPixelCount);
                    }
                    //Insert a zero pixel on the rightmost edge of the strip image
                    maSegPanorama[SLIDES + i].at<uchar>(j, utStripWidth-1) = 255;
//TODO: Need to check if the dummy pixel should 0 or 255?
                }
                itShortage --;
            }else{//PSS B is started from shift 22.5 degree to right. Need wrap up first and lst image strip
                //second set shifted 22.5 degree, hence doesn`t start from zero.
                for(int j = 0 ; j < uaHeight ; j ++){
                    //337.5 ~ 360 degree of panorama image
                    for(int k = 0 ; k < utStripWidth / 2 ; k ++){
                        maSegPanorama[SLIDES + i].at<uchar>(j, k ) = maInputPanorama.at<uchar>(j, utStripWidth/2 + k + utPixelCount);
                    }
                    //0 ~ 22.5 degree of panorama image
                    for(int k = utStripWidth / 2 ; k < utStripWidth ; k ++){
                        maSegPanorama[SLIDES + i].at<uchar>(j, k ) = maInputPanorama.at<uchar>(j, k - utStripWidth / 2);
                    }
                }
            }
        }else{//The origin width is the multiple of SLIDES.
            if( i != (SLIDES-1)){//
                for(int j = 0 ; j < uaHeight ; j ++){
                    for(int k = 0 ; k < utStripWidth ; k ++){
                        //second set shifted 22.5 degree, hence doesn`t start from zero.
                        maSegPanorama[SLIDES + i].at<uchar>(j, k ) = maInputPanorama.at<uchar>(j, utStripWidth/2 + k + utPixelCount);
                    }
                }
            }else if( i == (SLIDES-1)){
                //second set shifted 22.5 degree, hence doesn`t start from zero.
                for(int j = 0 ; j < uaHeight ; j ++){
                    //337.5 ~ 360 degree of panorama image
                    for(int k = 0 ; k < utStripWidth / 2 ; k ++){
                        maSegPanorama[SLIDES + i].at<uchar>(j, k ) = maInputPanorama.at<uchar>(j, utStripWidth/2 + k + utPixelCount);
                    }
                    //0 ~ 22.5 degree of panorama image
                    for(int k = utStripWidth / 2 ; k < utStripWidth ; k ++){
                        maSegPanorama[SLIDES + i].at<uchar>(j, k ) = maInputPanorama.at<uchar>(j, k - utStripWidth / 2);
                    }
                }
            }
        }
        //Update the width pixel counter
        utPixelCount = utPixelCount + utStripWidth;
#ifdef DEBUG01
        cv::imwrite("SEGMENT_IMAGE/SEGMENTED_" + alph + "_"  + std::to_string(i + SLIDES) + ".bmp", maSegPanorama[SLIDES + i]);
#endif
    }
    
#ifdef DEBUG01
    printf("Panorama width = %d & Counted pixels = %d\n", uaWidth, utPixelCount);
    printf("Strip width = %d & Shortage = %d\n", utStripWidth, iaShortage);
#endif
    return true;
}
  
