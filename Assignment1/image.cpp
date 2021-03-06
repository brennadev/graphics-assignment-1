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
    
    memcpy(data.raw, src.data.raw, num_pixels * 4);
    //*data.raw = *src.data.raw;
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
            scaled_p.r = ComponentClamp(factor*p.r);
            scaled_p.g = ComponentClamp(factor*p.g);
            scaled_p.b = ComponentClamp(factor*p.b);
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
            
            Pixel quantizationError = originalPixel - GetPixel(i, j);
            

            // spread the error depending on the location of the pixel (in case it's at the edges)
            // bottom right
            if (i == width - 1 && j == height - 1) {
                // TODO: fill this in
            // right edge
            } else if (i == width - 1) {
                GetPixel(i - 1, j + 1) = GetPixel(i - 1, j + 1) + quantizationError * (BETA + 4/16.0);
                GetPixel(i, j + 1) = GetPixel(i, j + 1) + quantizationError * (GAMMA + 4/16.0);
                
            // bottom edge
            } else if (j == height - 1) {
                GetPixel(i + 1, j) = GetPixel(i + 1, j) + quantizationError * (ALPHA + 9/16.0);
                
            // left edge
            } else if (i == 0) {
                GetPixel(i + 1, j) = GetPixel(i + 1, j) + quantizationError * (ALPHA + 1/16.0);
                GetPixel(i, j + 1) = GetPixel(i, j + 1) + quantizationError * (GAMMA + 1/16.0);
                GetPixel(i + 1, j + 1) = GetPixel(i + 1, j + 1) + quantizationError * (DELTA + 1/16.0);
                
            // anywhere else in image
            } else {
                GetPixel(i + 1, j) = GetPixel(i + 1, j) + quantizationError * ALPHA;
                GetPixel(i - 1, j + 1) = GetPixel(i - 1, j + 1) + quantizationError * BETA;
                GetPixel(i, j + 1) = GetPixel(i, j + 1) + quantizationError * GAMMA;
                GetPixel(i + 1, j + 1) = GetPixel(i + 1, j + 1) + quantizationError * DELTA;
            }
        }
    }
}


void Image::Blur(int n) {

    float filter[n][n];
    
    
    // set up the Gaussian filter
    
    // use an intermediary array to store the possible Gaussian values
    float intermediary[n / 2 + 1];
    
    // calculate all possible Gaussian values to later be put into the filter
    for (int i = 0; i <= n / 2; i++) {
        intermediary[i] = (1.0 / sqrt(2.0 * M_PI)) * pow(M_E, -1.0 * pow(i, 2.0) / 2.0);
        cout << "intermediary at " << i << " is " << intermediary[i] << endl;
    }
    
    // center position in the filter
    int center = n / 2;
    
    // needed for normalization
    float total = 0.0;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            cout << "intermediary value being retrieved for i = " << i << " and j = " << j << " is " << max(abs(center - i), abs(center - j)) << endl;
            filter[i][j] = intermediary[max(abs(center - i), abs(center - j))];
            total += filter[i][j];
        }
    }
    
    // normalize
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            filter[i][j] /= total;
            cout << "filter at x= " << i << " and y= " << j << " is " << filter[i][j] << endl;
        }
    }
    
    
    // when doing the convolution math, we always need to pull from the original image, not the partially blurred version of the original image
    Image *originalImage = new Image(*this);
    

    // for use after all pixels are read
    int filterTotalNumberOfElements = n * n;
    
    // actual convolution
    // go through each location in the image
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            
            // location of intermediate values after multiplication
            Pixel currentlyMultipliedByFilter[n][n];
            
            // so the correct location in the image is multiplied by the corresponding filter location - increment by 1 as necessary
            int currentImageLocationForFilterX = -1 * n / 2;
            int currentImageLocationForFilterY = -1 * n / 2;
            
            // go through each location in the filter and multiply
            for (int k = 0; k < n; k++) {
                for (int l = 0; l < n; l++) {
                    
                    // take care of the edges by extending the pixels closest to the edges
                    // start with default locations
                    int xLocationInImageToMultiply = i + currentImageLocationForFilterX;
                    int yLocationInImageToMultiply = j + currentImageLocationForFilterY;
                    
                    // if x goes off the left edge
                    if (i + currentImageLocationForFilterX < 0) {
                        xLocationInImageToMultiply = 0;
                        
                    // if x goes off the right edge
                    } else if (i + currentImageLocationForFilterX >= width) {
                        xLocationInImageToMultiply = width - 1;
                    }
                    
                    // if y goes off the top edge
                    if (j + currentImageLocationForFilterY < 0) {
                        yLocationInImageToMultiply = 0;
                        
                    // if y goes off the bottom edge
                    } else if (j + currentImageLocationForFilterY >= height) {
                        yLocationInImageToMultiply = height - 1;
                    }
                    
                    // the corrected locations get passed to the multiplication so it always will work
                    currentlyMultipliedByFilter[k][l] = originalImage->GetPixel(xLocationInImageToMultiply, yLocationInImageToMultiply) * filter[k][l];
                    
                    currentImageLocationForFilterY++;
                }
                currentImageLocationForFilterX++;
            }
            
            // due to clamping, averaging the components needs to be done by channel (ignoring alpha)
            int redTotal = 0;
            int greenTotal = 0;
            int blueTotal = 0;
            
            for (int k = 0; k < n; k++) {
                for (int l = 0; l < n; l++) {
                    redTotal += ComponentClamp(currentlyMultipliedByFilter[k][l].r);
                    greenTotal += ComponentClamp(currentlyMultipliedByFilter[k][l].g);
                    blueTotal += ComponentClamp(currentlyMultipliedByFilter[k][l].b);
                }
            }
            
            
            // actually take the averages - no clamping should be needed as all the original values are between 0 and 255
            redTotal /= filterTotalNumberOfElements;
            greenTotal /= filterTotalNumberOfElements;
            blueTotal /= filterTotalNumberOfElements;
            
            // finally, put it back in the original
            GetPixel(i, j).r = redTotal;
            GetPixel(i, j).g = greenTotal;
            GetPixel(i, j).b = blueTotal;
        }
    }
}


// TODO: test this once blur is implemented; I'm not sure if the interpolation amount will work
void Image::Sharpen(int n) {
    // we need to have a blurred copy of the image to work with
    Image blurredImage = Image(*this);
    blurredImage.Blur(n);
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            // I think the top one is the correct one (bottom one produces an all-black image)
            GetPixel(i, j) = PixelLerp(blurredImage.GetPixel(i, j), GetPixel(i, j), 2);
            //GetPixel(i, j) = PixelLerp(GetPixel(i, j), blurredImage.GetPixel(i, j), 2);
        }
    }
}


void Image::EdgeDetect() {
    
    // when doing the convolution math, we always need to pull from the original image, not the partially edge detected version of the original image
    Image *originalImage = new Image(*this);
    
    
    const int n = 3;

    // the filter for edge detect is always the same
    float filter[n][n] = {{ -1, -1, -1 },
                          { -1, 8, -1 },
                          { -1, -1, -1 }};
    
    int filterTotalNumberOfElements = n * n;
    
    // actual convolution - this section is identical to the blur code
    // go through each location in the image
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            
            // location of intermediate values after multiplication
            float currentlyMultipliedByFilterRed[n][n];
            float currentlyMultipliedByFilterGreen[n][n];
            float currentlyMultipliedByFilterBlue[n][n];
            
            // so the correct location in the image is multiplied by the corresponding filter location - increment by 1 as necessary
            int currentImageLocationForFilterX = -1 * n / 2;
            int currentImageLocationForFilterY = -1 * n / 2;
            
            // go through each location in the filter and multiply
            for (int k = 0; k < n; k++) {
                for (int l = 0; l < n; l++) {
                    
                    // take care of the edges by extending the pixels closest to the edges
                    // start with default locations
                    int xLocationInImageToMultiply = i + currentImageLocationForFilterX;
                    int yLocationInImageToMultiply = j + currentImageLocationForFilterY;
                    
                    // if x goes off the left edge
                    if (i + currentImageLocationForFilterX < 0) {
                        xLocationInImageToMultiply = 0;
                        
                        // if x goes off the right edge
                    } else if (i + currentImageLocationForFilterX >= width) {
                        xLocationInImageToMultiply = width - 1;
                    }
                    
                    // if y goes off the top edge
                    if (j + currentImageLocationForFilterY < 0) {
                        yLocationInImageToMultiply = 0;
                        
                        // if y goes off the bottom edge
                    } else if (j + currentImageLocationForFilterY >= height) {
                        yLocationInImageToMultiply = height - 1;
                    }
                    
                    // the corrected locations get passed to the multiplication so it always will work

                    // where I tried multiplying each channel individually so I knew that no clamping should be happening
                    currentlyMultipliedByFilterRed[k][l] = (originalImage->GetPixel(xLocationInImageToMultiply, yLocationInImageToMultiply).r) / 255.0 * filter[k][l];
                    currentlyMultipliedByFilterGreen[k][l] = (originalImage->GetPixel(xLocationInImageToMultiply, yLocationInImageToMultiply).g) / 255.0 * filter[k][l];
                    currentlyMultipliedByFilterBlue[k][l] = (originalImage->GetPixel(xLocationInImageToMultiply, yLocationInImageToMultiply).b) / 255.0 * filter[k][l];
                    
                    currentImageLocationForFilterY++;
                }
                currentImageLocationForFilterX++;
            }
            
            // due to clamping, averaging the components needs to be done by channel (ignoring alpha)
            float redTotal = 0.0;
            float greenTotal = 0.0;
            float blueTotal = 0.0;
            
            for (int k = 0; k < n; k++) {
                for (int l = 0; l < n; l++) {
                    redTotal += currentlyMultipliedByFilterRed[k][l];
                    greenTotal += currentlyMultipliedByFilterGreen[k][l];
                    blueTotal += currentlyMultipliedByFilterBlue[k][l];
                }
            }
            
            redTotal = ComponentClamp(redTotal);
            greenTotal = ComponentClamp(greenTotal);
            blueTotal = ComponentClamp(blueTotal);
            
            
            // actually take the averages - no clamping should be needed as all the original values are between 0 and 255
            redTotal /= filterTotalNumberOfElements;
            greenTotal /= filterTotalNumberOfElements;
            blueTotal /= filterTotalNumberOfElements;
            
            // finally, put it back in the original
            GetPixel(i, j).r = ComponentClamp(redTotal * 255.0);
            GetPixel(i, j).g = ComponentClamp(greenTotal * 255.0);
            GetPixel(i, j).b = ComponentClamp(blueTotal * 255.0);
        }
    }
}

// TODO: test this function; it should be complete, but Sample isn't implemented yet, so I can't test it
Image* Image::Scale(double sx, double sy) {
    
    // we need to an image the size of what the current image is after it's scaled
    Image *scaledImage = new Image(width * sx, height * sy);
    
    for (int i = 0; i < scaledImage->width; i++) {
        for (int j = 0; j < scaledImage->height; j++) {
            scaledImage->GetPixel(i, j) = Sample(i / sx, j / sy);
        }
    }
    
    return scaledImage;
}

// TODO: test this function; it should be complete, but Sample isn't implemented yet, so I can't test it
Image* Image::Rotate(double angle) {
    
    // just make a really large image
    Image *rotatedImage = new Image(1000, 1000);
    
    for (int i = 0; i < rotatedImage->width; i++) {
        for (int j = 0; j < rotatedImage->height; j++) {
            rotatedImage->GetPixel(i, j) = Sample(i * cos(-1 * angle) - j * sin(-1 * angle),
                                                  i * sin(-1 * angle) + j * cos(-1 * angle));
        }
    }
    
    
    
    return rotatedImage;
}


void Image::Fun() {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            GetPixel(i, j) = Sample(pow(i, 2), pow(j, 3));
        }
    }
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
            return GetPixel(u, v);
            break;
            
        case IMAGE_SAMPLING_BILINEAR:
            /*int uCeiling = ceil(u);
            int uFloor = floor(u);
            int vCeiling = ceil(v);
            int vFloor = ceil(v);*/
            
            //return PixelLerp(GetPixel(floor(u), floor(v)), GetPixel(ceil(u), ceil(v)), <#double t#>)
            break;
            
        case IMAGE_SAMPLING_GAUSSIAN:
            break;
    }
    return Pixel();
}
