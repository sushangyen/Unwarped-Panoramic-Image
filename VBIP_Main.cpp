//********************************************************************
// File name  : VBIP_MAIN.CPP
// Designer : Shang-Yen Su
// Create date: Apr. 26, 2017
// Description:
// This file is the main cpp file of VBIP.
// Naming convention:
// Prefix: s = string; i = integer; u = unisgned integer; f = float
//         d = double; m = dynamic memory; a = argument; t = temp
//********************************************************************

//#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string.h>
#include "header.h"

using namespace cv;
using namespace std;

#ifdef DEBUG_REF
FILE *fRefTableRow;
FILE *fRefTableRowA;
FILE *fRefTableRowB;
FILE *fRefTableCol;
FILE *fCheckCoordinates;
FILE *fCheckCoordinates_0;
FILE *fCheckAngle;
#endif

FILE *fMap;//File pointer of indoor map

// Three important global variables for the whole map building.
int SLIDES;
float DISTANCE; //The distance S between two corresponding panoramic image sets
float RADIUS = 0.03;//Set the radius in centermeter of the initial indoor map
double FOCAL;//Set the focal length of the camera
int TOTAL_TRACK;
int ACTIVE_TRACK;
int UPPER_HALF = NO;//The default half-plane detection
int NORMALIZE = NO;// The default setting of normalization

//read four panoramic images refA, refB, refC, refD
string sPanoramaPath[] = {"Panorama/refA.jpg", "Panorama/refB.jpg", "Panorama/refC.jpg", "Panorama/refD.jpg"}; //path to the panorama image A, B, C, D
//panorama image in grayscale mode
cv::Mat mPanoramaGray[4];
//width of input panorama image
unsigned int uiWidth[4];
//height of input panorama image
unsigned int uiHeight[4];

unsigned int uiTrackSize[4];
unsigned int uiTrim[4];
unsigned int uiNewHeight[4];

//Trim out the upper and lower part of the panorama pixel by pixel
cv::Mat mPanoramaImage[4];

bool ReadConfigure(){
    FILE *fConfig;//File pointer of VBIP configuration
    
    fConfig = fopen("VBIPMAP.cof", "r");
    char cRead[6];
    if(fConfig==NULL){
        printf("Missing VBIPMAP.cof configuration file!!\n");
        return false;
    }
    if(!fscanf(fConfig, "Slide = %d\n", &SLIDES)){
        printf("Configuration file format error - Slides!!\n");
        return false;
    };
    if(!fscanf(fConfig, "Distance = %f\n", &DISTANCE)){
        printf("Configuration file format error - Distance!!\n");
        return false;
    };
    if(!fscanf(fConfig, "Focal length = %lf\n", &FOCAL)){
        printf("Configuration file format error - Focal Length!!\n");
        return false;
    }
    if(fscanf(fConfig, "Trim off ratio (Active_Track/Total_Tracks) = %d/%d\n", &ACTIVE_TRACK, &TOTAL_TRACK)){
        if(ACTIVE_TRACK > TOTAL_TRACK){
            printf("Configuration file format error - TRACK_USE should NOT larger than IMAGE_TRACK!!\n");
            return false;
        }
    }else{
        printf("Configuration file format error - Trim off ratio!!\n");
        return false;
    }
    if(fscanf(fConfig, "Noramlize = %s\n", cRead)){
        if(!strncmp(cRead, "yes", 3) || !strncmp(cRead, "YES", 3)) NORMALIZE = YES;
        else if(!strncmp(cRead, "no", 2) || !strncmp(cRead, "NO", 2)) NORMALIZE = NO;
        else{
            printf("Configuration file format error - Normalize!!\n");
            return false;
        }
    }
    cRead[0] = {0};
    if(fscanf(fConfig, "Half-plane = %s\n", cRead)){
        if(!strncmp(cRead, "yes", 3) || !strncmp(cRead, "YES", 3)) UPPER_HALF = YES;
        else if(!strncmp(cRead, "no", 2) || !strncmp(cRead, "NO", 2)) UPPER_HALF = NO;
        else{
            printf("Configuration file format error - Half-plane!!\n");
            return false;
        }
    }
    
    fclose(fConfig);
    return true;
}

int main(int argc, char **argv){
    // Read the configuration of VBIP first!
    if(!ReadConfigure()) return -1;
    
#ifdef DEBUG01
    printf("SLIDES = %d\n", SLIDES);
    printf("Trim off ratio = %d/%d\n", ACTIVE_TRACK, TOTAL_TRACK);
    printf("NORMALIZE = %d\n", NORMALIZE);
    printf("UPPER_HALF = %d\n", UPPER_HALF);
#endif
    
    for(int k = 0; k < 4; k++){
        
        string alph;            //ascii to string
        alph= (char) k + 65;
        
        //1. Read panorama image from file in grayscale mode
        mPanoramaGray[k] = cv::imread(sPanoramaPath[k], CV_LOAD_IMAGE_GRAYSCALE);
        if(!mPanoramaGray[k].data){
            printf("Error!! Could not open or find the image\n");
            printf("Please also check if ""IndoorMap"" directory exist?\n");
            printf("Please confirm file format .png is required.\n");
            return -1;
        }
#ifdef CONSOLE
        //display input panorama for debug
        cv::imshow("show", mPanoramaGray[k]);
#endif
        uiWidth[k] = mPanoramaGray[k].cols;
        uiHeight[k] = mPanoramaGray[k].rows;
        //double dPixelR = (2 * PI * RADIUS)/(double)uiWidth;//Compute the radius of a pixel
        
        //Since the upper part of the panorama is the sky or ceiling and lower part
        //is the floor which are not useful for positioning, we will trim them out
        //and reform th epanorama to save memory and to reduce matching error.
        //We first compute the area that need to trim out.
        uiTrackSize[k] = uiHeight[k]/TOTAL_TRACK;//Height of each track
        uiTrim[k] = uiTrackSize[k] * (TOTAL_TRACK - ACTIVE_TRACK) / 2.0;
        uiNewHeight[k] = uiTrackSize[k] * (float)ACTIVE_TRACK;
        
        //Trim out the upper and lower part of the panorama pixel by pixel
        mPanoramaImage[k] = cv::Mat(uiNewHeight[k], uiWidth[k], CV_8UC1);
        for(int y = 0 ; y < uiNewHeight[k]; y++)
            for(int x = 0 ; x < uiWidth[k]; x++)
                mPanoramaImage[k].at<char>(y, x) = mPanoramaGray[k].at<char>(y+uiTrim[k], x);

#ifdef DEBUG01
        //cv::imwrite("Panorama/ref_resized_" + std::to_string(k) +".png", mPanoramaImage[k]);
        cv::imwrite("Panorama/ref_resized_" + alph +".png", mPanoramaImage[k]);
#endif
        
        // Check if the image width is the multiple of SLIDES.
        int iShortage = uiWidth[k] % SLIDES;
        if(iShortage > 0){
            //The width of the panorama is not the multiple of SLIDES
            //We need to pad more space to let width become multiple of SLIDES.
            uiWidth[k] += iShortage;
        }
        
        double dRadius = (double) uiWidth[k] / (2 * PI); //radius of the panorama image in pixels
        
#ifdef CONSOLE
        printf("Original Panorama: Width = %d & Height = %d\n", uiWidth[k], uiHeight[k]);
        printf("Trimmed Panorama: Width = %d & Height = %d & Shortage = %d & Radius = %f\n", uiWidth[k], uiNewHeight[k], iShortage, dRadius);
#endif
        
        //2. Segment the panorama into strip of images
        //cv::Mat mSegPanorama[SLIDES * 2];
        int iTotal = SLIDES * 2;
        cv::Mat *mSegPanorama = new cv::Mat [iTotal];
        //Cut the mPanoramaImage into iTotal number of segments and save them in the
        //array of mSegPanorama
        if(segment_Image(mPanoramaImage[k], mSegPanorama, uiWidth[k], uiNewHeight[k], iShortage, alph)){
#ifdef CONSOLE
            printf("Segment Image Success\n");
#endif
        }else{
            printf("Error!!! Panorama Segmentation Failure!!\n");
            return -1;// Exit program
        }

        //3. unwarped image to perspective view
        cv::Mat *unwPanorama = new cv::Mat [iTotal];
        //Unwarp strip images in mSegPanorama array into perspective images and
        //save them in the array, unwPanorama.
        if(unwarped_Image(mSegPanorama, unwPanorama, dRadius, uiWidth[k], uiHeight[k], uiNewHeight[k], alph)){
#ifdef CONSOLE
            printf("Unwarp Image Success!!!\n");
#endif
        }else{
            printf("Error!!! Perspective image upwarpping failure!!\n");
            return -1;
        }
        
        //Map build up with the unwarpped images;
        for(int i = 0; i < iTotal; i++){
            unwPanorama[i] = Init_mapCreate(unwPanorama[i], FOCAL, i, k, uiWidth[k], uiNewHeight[k]);
        }
        
#ifdef DEBUG_REF
        fclose(fRefTableRow);
        fclose(fRefTableRowA);
        fclose(fRefTableRowB);
        fclose(fRefTableCol);
        fclose(fCheckAngle);
        fclose(fCheckCoordinates);
        fclose(fCheckCoordinates_0);
        fclose(fMap);
#endif
    }

}
