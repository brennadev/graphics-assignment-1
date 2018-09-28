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
    // seed random number generator
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

void Image::Brighten (double factor) {
    int x,y;
    for (x = 0 ; x < Width() ; x++)
    {
        for (y = 0 ; y < Height() ; y++)
        {
            Pixel p = GetPixel(x, y);
            Pixel scaled_p;
            scaled_p.r = factor*p.r;
            scaled_p.g = factor*p.g;
            scaled_p.b = factor*p.b;
            GetPixel(x,y) = scaled_p;
        }
    }
}


void Image::ChangeContrast (double factor) {
    // calculate mean luminance
    float total = 0;

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            total += GetPixel(i, j).Luminance();
        }
    }
    
    // get final mean luminance and store it as a pixel
    float averageLuminance = total / num_pixels;
    Pixel luminancePixel = Pixel(averageLuminance, averageLuminance, averageLuminance, 1);
    
    // change each pixel
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            GetPixel(i, j) = PixelLerp(luminancePixel, GetPixel(i, j), factor);
        }
    }
}


void Image::ChangeSaturation(double factor) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            // calculate the luminance for the current pixel
            float luminance = GetPixel(i, j).Luminance();
            Pixel luminancePixel = Pixel(luminance, luminance, luminance, 1);
            
            GetPixel(i, j) = PixelLerp(luminancePixel, GetPixel(i, j), factor);
        }
    }
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


void Image::Quantize (int nbits) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            GetPixel(i, j) = PixelQuant(GetPixel(i, j), nbits);
        }
    }
}

// TODO: put image results on website
void Image::RandomDither (int nbits) {
    srand(time(NULL));
    
    // the area that the random value can fall into
    int areaWidth = 255.0 / nbits;
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            // start with a random value that falls in the color range and then convert it to a range of 0 to the first threshold
            float randomRed = (rand() % 255) % areaWidth;
            float randomGreen = (rand() % 255) % areaWidth;
            float randomBlue = (rand() % 255) % areaWidth;

            // now can do the actual calculation
            GetPixel(i, j).r = ComponentClamp(trunc(GetPixel(i, j).r + randomRed + 0.5));
            GetPixel(i, j).g = ComponentClamp(trunc(GetPixel(i, j).g + randomGreen + 0.5));
            GetPixel(i, j).b = ComponentClamp(trunc(GetPixel(i, j).b + randomBlue + 0.5));
        }
    }
    
}


static int Bayer4[4][4] =
{
    {15,  7, 13,  5},
    { 3, 11,  1,  9},
    {12,  4, 14,  6},
    { 0,  8,  2, 10}
};


void Image::OrderedDither(int nbits) {
    
}

/* Error-diffusion parameters */
const double
ALPHA = 7.0 / 16.0,
BETA  = 3.0 / 16.0,
GAMMA = 5.0 / 16.0,
DELTA = 1.0 / 16.0;

void Image::FloydSteinbergDither(int nbits){
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            // must save original pixel since the error is determined from that
            Pixel originalPixel = Pixel(GetPixel(i, j));
            
            // quantize first
            GetPixel(i, j) = PixelQuant(GetPixel(i, j), nbits);
            

            // spread the error depending on the location of the pixel (in case it's at the edges)
            // bottom right
            if (i == width - 1 && j == height - 1) {
                // TODO: fill this in
            // right edge
            } else if (i == width - 1) {
                GetPixel(i - 1, j + 1) = GetPixel(i - 1, j + 1) + originalPixel * (BETA + 4/16.0);
                GetPixel(i, j + 1) = GetPixel(i, j + 1) + originalPixel * (GAMMA + 4/16.0);
                
            // bottom edge
            } else if (j == height - 1) {
                GetPixel(i + 1, j) = GetPixel(i + 1, j) + originalPixel * (ALPHA + 9/16.0);
                
            // left edge
            } else if (i == 0) {
                GetPixel(i + 1, j) = GetPixel(i + 1, j) + originalPixel * (ALPHA + 1/16.0);
                GetPixel(i, j + 1) = GetPixel(i, j + 1) + originalPixel * (GAMMA + 1/16.0);
                GetPixel(i + 1, j + 1) = GetPixel(i + 1, j + 1) + originalPixel * (DELTA + 1/16.0);
                
            // anywhere else in image
            } else {
                GetPixel(i + 1, j) = GetPixel(i + 1, j) + originalPixel * ALPHA;
                GetPixel(i - 1, j + 1) = GetPixel(i - 1, j + 1) + originalPixel * BETA;
                GetPixel(i, j + 1) = GetPixel(i, j + 1) + originalPixel * GAMMA;
                GetPixel(i + 1, j + 1) = GetPixel(i + 1, j + 1) + originalPixel * DELTA;
            }
            
            // TODO: not sure if the clamping code is needed - just figured I'd try it just in case
            ComponentClamp(GetPixel(i, j).r);
            ComponentClamp(GetPixel(i, j).g);
            ComponentClamp(GetPixel(i, j).b);
        }
    }
}


void Image::Blur(int n) {
    
    float filter[3][3];
    
    // actual convolution
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            
            
            
            
            GetPixel(i + n / 2, j + n / 2) = GetPixel(i + n / 2, j + n / 2) * filter[i][j];
            
            // for values that are near the edge, make sure that that gets handled properly by extending the edge
            // when x is close to the edge
            /*if (i - n / 2 < 0) {
                <#statements#>
            }*/
            
            
        }
    }
    
    
    
    
    // TODO: a bunch of old code - not sure what of this I'll need to keep
    
    // Values that get passed to the Gaussian function later on
    /*float valuesToPassToGaussian[n];
    // The current location from 0 to add points at in valuesToPass
    float currentPoint;
    
    
    // must treat odd and even n differently
    if (n % 2 == 0) {
        currentPoint = 0.5;

        // Each point is going out from the center with a difference of 1, starting with 0.5 and -0.5. For example, if n == 6, then valuesToPass will be [-2.5, -1.5, -0.5, 0.5, 1.5, 2.5] after the loop finishes.
        for (int i = 0; i < (n - 2) / 2; i++) {
            valuesToPassToGaussian[n / 2 - 1 + i] = currentPoint;     // to right of center
            valuesToPassToGaussian[n / 2 - i] = -1 * currentPoint;    // to left of center
        }
    } else {
        // set the center point to 0
        valuesToPassToGaussian[n / 2] = 0;
        currentPoint = 1;
        
        // Each point is going out from the center with a difference of 1. The center point is at 0, so this means the next points are at -1 and 1. For example, if n == 7, then valuesToPass will be [-3, -2, -1, 0, 1, 2, 3] after the loop finishes.
        for (int i = 1; i <= (n - 1) / 2; i++) {
            valuesToPassToGaussian[n / 2 + i] = currentPoint;         // to right of center
            valuesToPassToGaussian[n / 2 - i] = -1 * currentPoint;    // to left of center
        }
    }
    
    // the 1D array that's filled after performing the Gaussian function on all positions
    float valuesAfterGaussian1D[n];
    
    // multiply each of these positions by the Gaussian function - always using a standard deviation of 1
    for (int i = 0; i < n; i++) {
        valuesAfterGaussian1D[i] = (1 / sqrt(2 * M_PI)) * pow(M_E, -1 * pow(valuesToPassToGaussian[i], 2) / 2);
    }

    // then we need to multiply these together to get the Gaussian filter that we'll work with
    // The Gaussian filter (populated after loop below)
    float gaussianFilter[n][n];
    
*/
}


// TODO: test this once blur is implemented; I'm not sure if the interpolation amount will work
void Image::Sharpen(int n) {
    // we need to have a blurred copy of the image to work with
    Image blurredImage = Image(*this);
    blurredImage.Blur(n);
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            GetPixel(i, j) = PixelLerp(GetPixel(i, j), blurredImage.GetPixel(i, j), 2);
        }
    }
}

void Image::EdgeDetect() {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            
        }
    }
}

// TODO: test this function; it should be complete, but Sample isn't implemented yet, so I can't test it
Image* Image::Scale(double sx, double sy) {
    
    // we want to make sure we don't modify the original image
    Image *scaledImage = new Image(*this);
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            scaledImage->GetPixel(i, j) = Sample(i / sx, j / sy);
        }
    }
    
    return scaledImage;
}

// TODO: test this function; it should be complete, but Sample isn't implemented yet, so I can't test it
Image* Image::Rotate(double angle) {
    // we want to make sure we don't modify the original image
    Image *rotatedImage = new Image(*this);
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            rotatedImage->GetPixel(i, j) = Sample(i * cos(-1 * angle) - j * sin(-1 * angle),
                                                  i * sin(-1 * angle) + j * cos(-1 * angle));
        }
    }
    
    return rotatedImage;
}

void Image::Fun() {
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

// TODO: finish this
Pixel Image::Sample (double u, double v){
    switch (sampling_method) {
        case IMAGE_SAMPLING_POINT:
            
            break;
            
        case IMAGE_SAMPLING_BILINEAR:
            break;
            
        case IMAGE_SAMPLING_GAUSSIAN:
            break;
    }
    return Pixel();
}
