// SigLib - .WAV file waterfall generation program
// Copyright (c) 2024 Delta Numerix All rights reserved.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <siglib.h>               // SigLib DSP library
#include <siglib_host_utils.h>    // Optionally includes conio.h and time.h subset functions

#define SAMPLE_LENGTH 1024
#define FFT_LENGTH SAMPLE_LENGTH
#define LOG2_FFT_LENGTH SAI_FftLengthLog2(FFT_LENGTH)    // Log2 FFT length,
#define RESULT_LENGTH (FFT_LENGTH >> 1)                  // Only need to store the lower 1/2 of the FFT output
#define OVERLAP_LENGTH (FFT_LENGTH >> 2)                 // 25 % overlap

static char WavFilename[80];

static SLWavFileInfo_s wavInfo;

int main(int argc, char* argv[])
{
  SLArrayIndex_t sampleCount;
  FILE* fpInputFile;
  FILE* fpOutputFile;
  SLArrayIndex_t FrameNumber = 0;
  SLArrayIndex_t OverlapSrcArrayIndex;
  SLData_t SampleRate;
  SLData_t WindowInverseCoherentGain;

  SLData_t* pDataArray = SUF_VectorArrayAllocate(SAMPLE_LENGTH);        // Input data array
  SLData_t* pOverlapArray = SUF_VectorArrayAllocate(OVERLAP_LENGTH);    // Overlap data array
  SLData_t* pWindowCoeffs = SUF_VectorArrayAllocate(FFT_LENGTH);        // Window coeffs data array
  SLData_t* pFDPRealData = SUF_VectorArrayAllocate(FFT_LENGTH);         // Real data array
  SLData_t* pFDPImagData = SUF_VectorArrayAllocate(FFT_LENGTH);         // Imaginary data array
  SLData_t* pFDPResults = SUF_VectorArrayAllocate(FFT_LENGTH);          // Results data array
  SLData_t* pFDPFFTCoeffs = SUF_FftCoefficientAllocate(FFT_LENGTH);     // FFT coefficient data array

  if ((NULL == pDataArray) || (NULL == pOverlapArray) || (NULL == pWindowCoeffs) || (NULL == pFDPRealData) || (NULL == pFDPImagData) ||
      (NULL == pFDPResults) || (NULL == pFDPFFTCoeffs)) {

    printf("Memory allocation error\n");
    exit(0);
  }

  SIF_CopyWithOverlap(&OverlapSrcArrayIndex);    // Pointer to source array index

  // Generate window table
  SIF_Window(pWindowCoeffs,                     // Window coefficients pointer
             SIGLIB_BLACKMAN_HARRIS_FOURIER,    // Window type
             SIGLIB_ZERO,                       // Window coefficient
             FFT_LENGTH);                       // Window length

  // Calculate window inverse coherent gain
  WindowInverseCoherentGain = SDA_WindowInverseCoherentGain(pWindowCoeffs,    // Pointer to source array
                                                            FFT_LENGTH);      // Window size

  // Initialise FFT
  SIF_Fft(pFDPFFTCoeffs,              // Pointer to FFT coefficients
          SIGLIB_BIT_REV_STANDARD,    // Bit reverse mode flag / Pointer to bit
                                      // reverse address table
          FFT_LENGTH);                // FFT length

  if (argc != 2) {
    printf("\nUsage error:\nwavwfall filename (no extension)\n\n");
    exit(-1);    // Exit - usage error
  }

  strcpy(WavFilename, argv[1]);
  strcat(WavFilename, ".wav");

  printf("Source file = %s\n", WavFilename);

  if ((fpInputFile = fopen(WavFilename, "rb")) == NULL) {    // Note this file is binary
    printf("Error opening input .WAV file\n");
    exit(-1);
  }

  if ((fpOutputFile = fopen("sc.gpdt", "w")) == NULL) {
    printf("Error opening output file\n");
    exit(-1);
  }

  wavInfo = SUF_WavReadHeader(fpInputFile);
  if (wavInfo.NumberOfChannels != 1) {    // Check how many channels
    printf("Number of channels in %s = %d\n", WavFilename, wavInfo.NumberOfChannels);
    printf("This app requires a mono .wav file\n");
    exit(-1);
  }

  SUF_WavDisplayInfo(wavInfo);

  SampleRate = wavInfo.SampleRate;

  fprintf(fpOutputFile, "# 3D plot for %s\n\n", WavFilename);
  fprintf(fpOutputFile, "# Time\t\tFrequency\tMagnitude\n\n");

  while ((sampleCount = SUF_WavReadData(pDataArray, fpInputFile, wavInfo, SAMPLE_LENGTH)) == SAMPLE_LENGTH) {
    // Apply the overlap to the data
    while (SDA_CopyWithOverlap(pDataArray,               // Pointer to source array
                               pFDPRealData,             // Pointer to destination array
                               pOverlapArray,            // Pointer to overlap array
                               &OverlapSrcArrayIndex,    // Pointer to source array index
                               SAMPLE_LENGTH,            // Source array length
                               OVERLAP_LENGTH,           // Overlap length
                               SAMPLE_LENGTH) <          // Destination array length
           SAMPLE_LENGTH) {

      // Apply window to real data
      SDA_Window(pFDPRealData,             // Pointer to source array
                 pFDPRealData,             // Pointer to destination array
                 pWindowCoeffs,            // Window array pointer
                 FFT_LENGTH);              // Window size
                                           // Perform FFT
      SDA_Rfft(pFDPRealData,               // Real array pointer
               pFDPImagData,               // Pointer to imaginary array
               pFDPFFTCoeffs,              // Pointer to FFT coefficients
               SIGLIB_BIT_REV_STANDARD,    // Bit reverse mode flag / Pointer to
                                           // bit reverse address table
               FFT_LENGTH,                 // FFT length
               LOG2_FFT_LENGTH);           // log2 FFT length

      // Normalize FFT scaling value
      SDA_ComplexScalarMultiply(pFDPRealData,                                                         // Pointer to real source array
                                pFDPImagData,                                                         // Pointer to imaginary source array
                                (SIGLIB_TWO * WindowInverseCoherentGain) / ((SLData_t)FFT_LENGTH),    // Multiplier
                                pFDPRealData,                                                         // Pointer to real destination array
                                pFDPImagData,                                                         // Pointer to imaginary destination array
                                FFT_LENGTH);                                                          // Array lengths

      // Calc real power fm complex
      SDA_LogMagnitude(pFDPRealData,      // Pointer to real source array
                       pFDPImagData,      // Pointer to imaginary source array
                       pFDPResults,       // Pointer to log magnitude destination array
                       RESULT_LENGTH);    // Array length

      // Clip signal to improve graph
      SDA_Clip(pFDPResults,          // Source array address
               pFDPResults,          // Destination array address
               SIGLIB_ZERO,          // Value to clip signal to
               SIGLIB_CLIP_BELOW,    // Clip type
               RESULT_LENGTH);       // Array length

      // Store data to GNUPlot file - Frequency in KHz
      for (SLArrayIndex_t i = 0; i < RESULT_LENGTH; i++) {
        fprintf(fpOutputFile, "%lf\t%lf\t%lf\n", ((SLData_t)FrameNumber) * (((SLData_t)(SAMPLE_LENGTH - OVERLAP_LENGTH)) / SampleRate),
                (((SLData_t)i) * (SampleRate / ((SLData_t)FFT_LENGTH)) / 1000.0), *(pFDPResults + i));
      }
      fprintf(fpOutputFile, "\n");

      FrameNumber++;
    }
  }

  fclose(fpInputFile);    // Close files
  fclose(fpOutputFile);

  free(pDataArray);    // Free memory
  free(pOverlapArray);
  free(pWindowCoeffs);
  free(pFDPRealData);
  free(pFDPImagData);
  free(pFDPResults);
  free(pFDPFFTCoeffs);

  return (0);
}
