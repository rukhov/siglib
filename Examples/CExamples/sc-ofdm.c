// SigLib SC-OFDM DFT Example
// Symbol length = 32 samples
// Sub-carriers filled = 3 to 7
// LTE Symbol period = 15 kHz = 66.7 us
//     Requires radix 2, 3 and 5 DFTs
// Copyright (c) 2024 Delta Numerix All rights reserved.

// Include files
#include <stdio.h>
#include <siglib.h>             // SigLib DSP library
#include <gnuplot_c.h>          // Gnuplot/C
#include "plot_fd/plot_fd.h"    // Frequency domain plots

// Define constants
#define DFT_LENGTH 5
#define SYMBOL_LENGTH 32
#define FRAME_SIZE (SYMBOL_LENGTH * DFT_LENGTH)
#define FIRST_SUB_CARRIER 3

// Declare global variables and arrays

int main(void)
{
  h_GPC_Plot* h2DPlot;    // Plot object
  SLData_t CarrierSign;

  SLData_t* pSrcReal = SUF_VectorArrayAllocate(SYMBOL_LENGTH);    // Allocate memory
  SLData_t* pSrcImag = SUF_VectorArrayAllocate(SYMBOL_LENGTH);
  SLData_t* pDftReal = SUF_VectorArrayAllocate(SYMBOL_LENGTH);
  SLData_t* pDftImag = SUF_VectorArrayAllocate(SYMBOL_LENGTH);
  SLData_t* pDstReal = SUF_VectorArrayAllocate(SYMBOL_LENGTH);
  SLData_t* pDstImag = SUF_VectorArrayAllocate(SYMBOL_LENGTH);

  h2DPlot =                           // Initialize plot
      gpc_init_2d("SC-OFDM",          // Plot title
                  "Time",             // X-Axis label
                  "Magnitude",        // Y-Axis label
                  GPC_AUTO_SCALE,     // Scaling mode
                  GPC_SIGNED,         // Sign mode
                  GPC_KEY_ENABLE);    // Legend / key mode
  if (NULL == h2DPlot) {
    printf("\nPlot creation failure.\n");
    exit(-1);
  }

  SDA_Zeros(pSrcReal, SYMBOL_LENGTH);    // Clear data arrays
  SDA_Zeros(pSrcImag, SYMBOL_LENGTH);
  SDA_Zeros(pDstReal, SYMBOL_LENGTH);
  SDA_Zeros(pDstImag, SYMBOL_LENGTH);

  CarrierSign = SIGLIB_ONE;

  // Create sub-carriers
  for (SLFixData_t i = 0; i < DFT_LENGTH; i++) {
    printf("\nNumber of sub-carriers = %d\n", i + 1);

    *(pSrcReal + FIRST_SUB_CARRIER + i) = CarrierSign;    // Modulate carrier
    //      CarrierSign *= SIGLIB_MINUS_ONE;            // Uncomment to modulate
    //      carrier sign

    // Use a DFT in place of the FFT to save having to generate the coefficients
    // etc.
    SDA_Ridft(pSrcReal,          // Pointer to real source array
              pDstReal,          // Pointer to real destination array
              pDstImag,          // Pointer to imaginary destination array
              SYMBOL_LENGTH);    // Transform size

    gpc_plot_2d(h2DPlot,                        // Graph handle
                pDstReal,                       // Dataset
                SYMBOL_LENGTH,                  // Dataset length
                "Modulated signal",             // Dataset title
                SIGLIB_ZERO,                    // Minimum X value
                (double)(SYMBOL_LENGTH - 1),    // Maximum X value
                "lines",                        // Graph type
                "magenta",                      // Colour
                GPC_NEW);                       // New graph
    gpc_plot_2d(h2DPlot,                        // Graph handle
                pDstImag,                       // Dataset
                SYMBOL_LENGTH,                  // Dataset length
                "Modulated signal",             // Dataset title
                SIGLIB_ZERO,                    // Minimum X value
                (double)(SYMBOL_LENGTH - 1),    // Maximum X value
                "lines",                        // Graph type
                "blue",                         // Colour
                GPC_ADD);                       // New graph
    printf("\nModulated signal\n");

    plot_complex_frequency_domain(pDstReal, pDstImag, SIGLIB_RECTANGLE_FOURIER, "Raw signal spectrum", SYMBOL_LENGTH, SYMBOL_LENGTH);

    printf("Peak to average power ratio (linear) without DFT = %lf\n",
           SDA_PeakToAveragePowerRatioComplex(pDstReal,           // Pointer to real source array
                                              pDstImag,           // Pointer to imaginary source array
                                              SYMBOL_LENGTH));    // Dataset length
    printf("Peak to average power ratio (dB) without DFT = %lf\n",
           SDA_PeakToAveragePowerRatioComplexDB(pDstReal,           // Pointer to real source array
                                                pDstImag,           // Pointer to imaginary source array
                                                SYMBOL_LENGTH));    // Dataset length
    printf("\nPlease hit <Carriage Return> to continue . . .");
    getchar();

    // Perform DFT
    SDA_Zeros(pDftReal, SYMBOL_LENGTH);    // Clear FFT source data arrays
    SDA_Zeros(pDftImag, SYMBOL_LENGTH);

    SDA_Cdft(pSrcReal + FIRST_SUB_CARRIER,    // Pointer to real source array
             pSrcImag + FIRST_SUB_CARRIER,    // Pointer to imaginary source array
             pDftReal + FIRST_SUB_CARRIER,    // Pointer to real destination array
             pDftImag + FIRST_SUB_CARRIER,    // Pointer to imaginary destination array
             DFT_LENGTH);                     // Transform size

    // Use a DFT in place of the FFT to save having to generate the coefficients
    // etc.
    SDA_Cidft(pDftReal,          // Pointer to real source array
              pDftImag,          // Pointer to imaginary source array
              pDstReal,          // Pointer to real destination array
              pDstImag,          // Pointer to imaginary destination array
              SYMBOL_LENGTH);    // Transform size

    gpc_plot_2d(h2DPlot,                        // Graph handle
                pDstReal,                       // Dataset
                SYMBOL_LENGTH,                  // Dataset length
                "DFT Modulated signal",         // Dataset title
                SIGLIB_ZERO,                    // Minimum X value
                (double)(SYMBOL_LENGTH - 1),    // Maximum X value
                "lines",                        // Graph type
                "magenta",                      // Colour
                GPC_NEW);                       // New graph
    gpc_plot_2d(h2DPlot,                        // Graph handle
                pDstImag,                       // Dataset
                SYMBOL_LENGTH,                  // Dataset length
                "DFT Modulated signal",         // Dataset title
                SIGLIB_ZERO,                    // Minimum X value
                (double)(SYMBOL_LENGTH - 1),    // Maximum X value
                "lines",                        // Graph type
                "blue",                         // Colour
                GPC_ADD);                       // New graph
    printf("\nDFT Modulated signal\n");

    plot_complex_frequency_domain(pDstReal, pDstImag, SIGLIB_RECTANGLE_FOURIER, "SC signal spectrum", SYMBOL_LENGTH, SYMBOL_LENGTH);

    printf("Peak to average power ratio (linear) with DFT = %lf\n",
           SDA_PeakToAveragePowerRatioComplex(pDstReal,           // Pointer to real source array
                                              pDstImag,           // Pointer to imaginary source array
                                              SYMBOL_LENGTH));    // Dataset length
    printf("Peak to average power ratio (dB) with DFT = %lf\n",
           SDA_PeakToAveragePowerRatioComplexDB(pDstReal,           // Pointer to real source array
                                                pDstImag,           // Pointer to imaginary source array
                                                SYMBOL_LENGTH));    // Dataset length
    printf("\nPlease hit <Carriage Return> to continue . . .");
    getchar();
  }

  printf("\nHit <Carriage Return> to continue ....\n");
  getchar();    // Wait for <Carriage Return>
  gpc_close(h2DPlot);

  SUF_MemoryFree(pSrcReal);    // Free memory
  SUF_MemoryFree(pSrcImag);
  SUF_MemoryFree(pDftReal);
  SUF_MemoryFree(pDftImag);
  SUF_MemoryFree(pDstReal);
  SUF_MemoryFree(pDstImag);

  return (0);
}
