//
//  Image.cpp
//  Assignment1
//
//  Created by Brenna Olson on 9/20/18.
//  Copyright © 2018 Brenna Olson. All rights reserved.
//

#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <iostream>
//#include <cstdlib>
//#include <ctime>
using namespace std;

/**
 * Image
 **/
Image::Image (int width_, int height_){
    
    assert(width_ > 0);
    assert(height_ > 0);
    
    width           = width_;
    height          = height_;
    num_pixels      = width * height;
    sampling_method = IMAGE_SAMPLING_POINT;
    
    data.raw = new uint8_t[num_pixels*4];
    int b = 0; //which byte to write to
    for (int j = 0; j < height; j++){
        for (int i = 0; i < width; i++){
            data.raw[b++] = 0;
            data.raw[b++] = 0;
            data.raw[b++] = 0;
            data.raw[b++] = 0;
        }
    }
    
    assert(data.raw != NULL);
}

Image::Image (const Image& src){
    
    width           = src.width;
    height          = src.height;
    num_pixels      = width * height;
    sampling_method = IMAGE_SAMPLING_POINT;
    
    data.raw = new uint8_t[num_pixels*4];
    
    //memcpy(data.raw, src.data.raw, num_pixels);
    *data.raw = *src.data.raw;
}

Image::Image (char* fname){
    
    int numComponents; //(e.g., Y, YA, RGB, or RGBA)
    data.raw = stbi_load(fname, &width, &height, &numComponents, 4);
    
    if (data.raw == NULL){
        printf("Error loading image: %s", fname);
        exit(-1);
    }
    
    
    num_pixels = width * height;
    sampling_method = IMAGE_SAMPLING_POINT;
    
}

Image::~Image (){
    delete data.raw;
    data.raw = NULL;
}

void Image::Write(char* fname){
    
    int lastc = strlen(fname);
    
    switch (fname[lastc-1]){
        case 'g': //jpeg (or jpg) or png
            if (fname[lastc-2] == 'p' || fname[lastc-2] == 'e') //jpeg or jpg
                stbi_write_jpg(fname, width, height, 4, data.raw, 95);  //95% jpeg quality
            else //png
                stbi_write_png(fname, width, height, 4, data.raw, width*4);
            break;
        case 'a': //tga (targa)
            stbi_write_tga(fname, width, height, 4, data.raw);
            break;
        case 'p': //bmp
        default:
            stbi_write_bmp(fname, width, height, 4, data.raw);
    }
}

void Image::AddNoise (double factor) {
    srand(time(NULL));
    
    // don't want to add any noise if the factor is 0
    if (factor == 0.0) {
        return;
    }
    
    // want to modify half of the total pixels in the image
    for(int i = 0; i < num_pixels * (factor / 2); i++) {
        // randomly select the pixels to modify
        data.pixels[rand() % num_pixels] = PixelRandom();
    }
}

void Image::Brighten (double factor)
{
    int x,y;
    for (x = 0 ; x < Width() ; x++)
    {
        for (y = 0 ; y < Height() ; y++)
        {
            Pixel p = GetPixel(x, y);
            Pixel scaled_p = p*factor;
            GetPixel(x,y) = scaled_p;
        }
    }
}


void Image::ChangeContrast (double factor)
{
    // calculate mean luminance
    float total = 0;
    
    for(int i = 0; i < num_pixels; i++) {
        total += 0.3 * data.pixels[i].r + 0.59 * data.pixels[i].g + 0.11 * data.pixels[i].b;
    }
    
    // get final mean luminance
    float averageLuminance = total / num_pixels;
    Pixel luminancePixel = Pixel(averageLuminance, averageLuminance, averageLuminance, 1);
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            GetPixel(i, j) = PixelLerp(luminancePixel, GetPixel(i, j), factor);
        }
    }
}


void Image::ChangeSaturation(double factor)
{
    /* WORK HERE */
}


Image* Image::Crop(int x, int y, int w, int h)
{
    // the final cropped version of the original image
    Image *croppedImage = new Image(w, h);
    
    for(int j = y; j < y + h; j++) {
        for(int i = x; i < x + w; i++) {
            // starting at the (i, j) in the original image and at the origin in the cropped image - getting/setting locations relative to those points
            croppedImage->SetPixel(i - x, j - y, GetPixel(i, j));
        }
    }
    return croppedImage;
}


void Image::ExtractChannel(int channel) {
    // go through all pixels
    for(int i = 0; i < num_pixels; i++) {
        // set the channels not the one passed in to 0
        switch (channel) {
            case IMAGE_CHANNEL_RED:
                data.pixels[i].g = 0;
                data.pixels[i].b = 0;
                break;
            case IMAGE_CHANNEL_GREEN:
                data.pixels[i].r = 0;
                data.pixels[i].b = 0;
                break;
            case IMAGE_CHANNEL_BLUE:
                data.pixels[i].r = 0;
                data.pixels[i].g = 0;
                break;
        }
    }
}


void Image::Quantize (int nbits)
{
    /* WORK HERE */
}

void Image::RandomDither (int nbits)
{
    /* WORK HERE */
}


static int Bayer4[4][4] =
{
    {15,  7, 13,  5},
    { 3, 11,  1,  9},
    {12,  4, 14,  6},
    { 0,  8,  2, 10}
};


void Image::OrderedDither(int nbits)
{
    /* WORK HERE */
}

/* Error-diffusion parameters */
const double
ALPHA = 7.0 / 16.0,
BETA  = 3.0 / 16.0,
GAMMA = 5.0 / 16.0,
DELTA = 1.0 / 16.0;

void Image::FloydSteinbergDither(int nbits)
{
    /* WORK HERE */
}

void Image::Blur(int n)
{
    /* WORK HERE */
}

void Image::Sharpen(int n)
{
    /* WORK HERE */
}

void Image::EdgeDetect()
{
    /* WORK HERE */
}

Image* Image::Scale(double sx, double sy)
{
    /* WORK HERE */
    return NULL;
}

Image* Image::Rotate(double angle)
{
    /* WORK HERE */
    return NULL;
}

void Image::Fun()
{
    /* WORK HERE */
}

/**
 * Image Sample
 **/
void Image::SetSamplingMethod(int method)
{
    assert((method >= 0) && (method < IMAGE_N_SAMPLING_METHODS));
    sampling_method = method;
}


Pixel Image::Sample (double u, double v){
    /* WORK HERE */
    return Pixel();
}
