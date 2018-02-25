
#include "Weather3D.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include "math.h"
#include "float.h"
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

const bool saveToFile = true;


///////////////////////////////////////
// VARIABLES

int currInd = 0;
bool firstTime = true;

float dT;
int gridX;
int gridY;
int gridZ;
float gridSizeI;
float gridSizeJ;
std::vector<float> gridSizeK;

Grid3D grid3D[3];
Grid3D gridInit;
Grid3D gridRslow;
Ground3D ground;
RadStruct radStruct;

/* Copy to the structs the current state of the simulation */
void copyCurrentState(Grid3D& _currGrid, Ground3D& _currGround) {
  _currGrid = grid3D[currInd];
  _currGround = ground;
}

/* Saves to hard drive the current state of the selected variables at different heights to be ploted */
void saveToHDDCurrentState() {
  const int vars[] = {U, V, W, Pi, THETA, QV, QR};// Variables to save: U V W PI T QV QR
  std::vector<int> heightToSave = {2, gridZ / 2, gridZ - 2}; // Sample Pos: close to bottom / middle / close to top

  std::ofstream oU;
  oU.open("OUT.csv", std::fstream::out | std::fstream::app);
  const int size = (sizeof(vars) / sizeof(int));
  for (int k = 1; k < gridZ - 1; k++) {
    for (int j = 0; j < gridY; j++) {
      for (int i = 0; i < gridX; i++) {
        oU << "[" << i << ", " << j << ", " << k << "] ";
        for (int vn = 0; vn < size; vn++) {
          if (grid3D[currInd](vars[vn], i, j, k) != grid3D[currInd](vars[vn], i, j, k) || gridRslow(vars[vn], i, j, k) != gridRslow(vars[vn], i, j, k)) {
            // NaN ERROR
            printf("ERROR: Variable Nan--> Exit\n");
            oU << "\n"; oU.close();
            exit(0);
          }
          oU << "," << grid3D[currInd](vars[vn], i, j, k) << "," << gridRslow(vars[vn], i, j, k);
        }
        oU << "\n";
      }
    }
  }
  oU.close();
}


///////////////////////////////////////
// SIMULATION STEPS

/* STEP 1 Fundamental Equations */
void simulateSTEP1(
  const float dT, const int gridX, const int gridY, const int gridZ, const float simulationTime,
  const float gridSizeI, const float gridSizeJ, float *gridSizeK,
  Grid3D& prevGC, Grid3D& currGC, Grid3D& nextGC,
  Grid3D& gridRslow, Grid3D& gridInit) {

  for (int k = 1; k < gridZ - 1; k++) {
    for (int j = 0; j < gridY; j++) {
      for (int i = 0; i < gridX; i++) {

        int km1 = k - 1;
        if (km1 < 0) km1 = 0;

        const float Kx = 500.0f; // diffusion coefficients
        const float Kz = 100.0f;

        gridRslow(U, i, j, k) =
          -1.0f / gridSizeI * (powf(0.5f * (currGC(U, i + 1, j, k) + currGC(U, i, j, k)), 2) - powf(0.5f * (currGC(U, i, j, k) + currGC(U, i - 1, j, k)), 2)) // -duu/dx

          - 1.0f / gridSizeJ * (
          0.5f * (currGC(U, i, j + 1, k) + currGC(U, i, j, k)) * 0.5f * (currGC(V, i, j + 1, k) + currGC(V, i - 1, j + 1, k))
          - 0.5f * (currGC(U, i, j, k) + currGC(U, i, j - 1, k)) * 0.5f * (currGC(V, i, j, k) + currGC(V, i - 1, j, k)))// -duv/dy

          - 1.0f / (currGC(RO, i, j, k) * gridSizeK[k]) * (
          0.5f * (currGC(RO, i, j, k + 1) + currGC(RO, i, j, k)) * 0.5f * (currGC(W, i, j, k + 1) + currGC(W, i - 1, j, k + 1)) * 0.5f * (currGC(U, i, j, k + 1) + currGC(U, i, j, k))
          - 0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) * 0.5f * (currGC(W, i, j, k) + currGC(W, i - 1, j, k)) * 0.5f * (currGC(U, i, j, k - 1) + currGC(U, i, j, k))) // -dpuw/dz
          - 1.0f / gridSizeI * (cpd * (gridInit(THETA, i, j, k) + 0.61f * gridInit(QV, i, j, k)) * (currGC(Pi, i, j, k) - currGC(Pi, i - 1, j, k))) // -cpd*T*dP/dx
          + Kx / powf(gridSizeI, 2) * (prevGC(U, i + 1, j, k) - 2.0f * prevGC(U, i, j, k) + prevGC(U, i - 1, j, k))
          + Ky / powf(gridSizeJ, 2) * (prevGC(U, i, j + 1, k) - 2.0f * prevGC(U, i, j, k) + prevGC(U, i, j - 1, k))
          + Kz / powf(gridSizeK[k], 2) * ((prevGC(U, i, j, k + 1) - gridInit(U, i, j, k + 1)) - 2.0f * (prevGC(U, i, j, k) - gridInit(U, i, j, k)) + (prevGC(U, i, j, k - 1) - gridInit(U, i, j, k - 1))); // Diffusion (implicit)


        gridRslow(V, i, j, k) =
          -1.0f / gridSizeI * (
          0.5f * (currGC(V, i + 1, j, k) + currGC(V, i, j, k)) * 0.5f * (currGC(U, i + 1, j, k) + currGC(U, i + 1, j - 1, k))
          - 0.5f * (currGC(V, i, j, k) + currGC(V, i - 1, j, k)) * 0.5f * (currGC(U, i, j - 1, k) + currGC(U, i, j, k)))// -dvu/dx 

          - 1.0f / gridSizeJ * (powf(0.5f * (currGC(V, i, j + 1, k) + currGC(V, i, j, k)), 2) - powf(0.5f * (currGC(V, i, j, k) + currGC(V, i, j - 1, k)), 2)) // -dvv/dy

          - 1.0f / (currGC(RO, i, j, k) * gridSizeK[k]) * (
          0.5f * (currGC(RO, i, j, k + 1) + currGC(RO, i, j, k)) * 0.5f * (currGC(W, i, j, k + 1) + currGC(W, i, j - 1, k + 1)) * 0.5f * (currGC(V, i, j, k + 1) + currGC(V, i, j, k))
          - 0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) * 0.5f * (currGC(W, i, j, k) + currGC(W, i, j - 1, k)) * 0.5f * (currGC(V, i, j, k - 1) + currGC(V, i, j, k))) // -dpvw/dz
          - 1.0f / gridSizeJ * (cpd * (gridInit(THETA, i, j, k) + 0.61f * gridInit(QV, i, j, k)) * (currGC(Pi, i, j, k) - currGC(Pi, i, j - 1, k))) // -cpd*T*dP/dx
          + Kx / powf(gridSizeI, 2) * (prevGC(V, i + 1, j, k) - 2.0f * prevGC(V, i, j, k) + prevGC(V, i - 1, j, k))
          + Ky / powf(gridSizeJ, 2) * (prevGC(V, i, j + 1, k) - 2.0f * prevGC(V, i, j, k) + prevGC(V, i, j - 1, k))
          + Kz / powf(gridSizeK[k], 2) * ((prevGC(V, i, j, k + 1) - gridInit(V, i, j, k + 1)) - 2.0f * (prevGC(V, i, j, k) - gridInit(V, i, j, k)) + (prevGC(V, i, j, k - 1) - gridInit(V, i, j, k - 1))); // Diffusion (implicit)

        gridRslow(W, i, j, k) =
          -1.0f / gridSizeI * (
          0.5f * (currGC(U, i + 1, j, k) + currGC(U, i + 1, j, k - 1)) * 0.5f * (currGC(W, i + 1, j, k) + currGC(W, i, j, k))
          - 0.5f * (currGC(U, i, j, k) + currGC(U, i, j, k - 1)) * 0.5f * (currGC(W, i, j, k) + currGC(W, i - 1, j, k))) // -duw/dx

          - 1.0f / gridSizeJ * (
          0.5f * (currGC(V, i, j + 1, k) + currGC(V, i, j + 1, k - 1)) * 0.5f * (currGC(W, i, j + 1, k) + currGC(W, i, j, k))
          - 0.5f * (currGC(V, i, j, k) + currGC(V, i, j, k - 1)) * 0.5f * (currGC(W, i, j, k) + currGC(W, i, j - 1, k))) // -duw/dx

          - 1.0f / (0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) * 0.5f * (gridSizeK[k] + gridSizeK[km1])) *
          (currGC(RO, i, j, k) * powf(0.5f * (currGC(W, i, j, k + 1) + currGC(W, i, j, k)), 2) - currGC(RO, i, j, k - 1) * powf(0.5f * (currGC(W, i, j, k) + currGC(W, i, j, k - 1)), 2)) // -dpww/dz
          - 1.0f / (0.5f * (gridSizeK[k] + gridSizeK[km1])) *
          (cpd * 0.5f * (gridInit(THETA, i, j, k) + 0.61f * gridInit(QV, i, j, k) + gridInit(THETA, i, j, k - 1) + 0.61f * gridInit(QV, i, j, k - 1))
          * (currGC(Pi, i, j, k) - currGC(Pi, i, j, k - 1))) // -cpd* T*dP/dz
          + g  * 0.5f * (currGC(THETA, i, j, k) / gridInit(THETA, i, j, k) + currGC(THETA, i, j, k - 1) / gridInit(THETA, i, j, k - 1)
          + 0.61f* (currGC(QV, i, j, k) + currGC(QV, i, j, k - 1)) - (currGC(QC, i, j, k) + currGC(QC, i, j, k - 1) + currGC(QR, i, j, k) + currGC(QR, i, j, k - 1))) // B=g* T'/T
          + Kx / powf(gridSizeI, 2) * (prevGC(W, i + 1, j, k) - 2.0f * prevGC(W, i, j, k) + prevGC(W, i - 1, j, k)) // Diffusion (implicit)
          + Ky / powf(gridSizeJ, 2) * (prevGC(W, i, j + 1, k) - 2.0f * prevGC(W, i, j, k) + prevGC(W, i, j - 1, k)) // Diffusion (implicit)
          + Kz / powf(gridSizeK[k], 2) * (prevGC(W, i, j, k + 1) - 2.0f * prevGC(W, i, j, k) + prevGC(W, i, j, k - 1)); // d2w/dx2+d2w/dz2

        gridRslow(THETA, i, j, k) =
          -1.0f / gridSizeI * (currGC(U, i + 1, j, k) * 0.5f * (currGC(THETA, i + 1, j, k) + currGC(THETA, i, j, k))
          - currGC(U, i, j, k) * 0.5f * (currGC(THETA, i, j, k) + currGC(THETA, i - 1, j, k))) // -duT/dx

          - 1.0f / gridSizeJ * (currGC(V, i, j + 1, k) * 0.5f * (currGC(THETA, i, j + 1, k) + currGC(THETA, i, j, k))
          - currGC(V, i, j, k) * 0.5f * (currGC(THETA, i, j, k) + currGC(THETA, i, j - 1, k))) // -dvT/dx NN

          - 1.0f / (currGC(RO, i, j, k) * gridSizeK[k]) * (
          0.5f * (currGC(RO, i, j, k + 1) + currGC(RO, i, j, k)) * currGC(W, i, j, k + 1) * 0.5f * (currGC(THETA, i, j, k + 1) + currGC(THETA, i, j, k))
          - 0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) * currGC(W, i, j, k) * 0.5f * (currGC(THETA, i, j, k) + currGC(THETA, i, j, k - 1))) // -dpwT/dz
          - 1.0f / (currGC(RO, i, j, k)) * 0.5f * (
          0.5f * (currGC(RO, i, j, k + 1) + currGC(RO, i, j, k)) * currGC(W, i, j, k + 1) * (gridInit(THETA, i, j, k + 1) - gridInit(THETA, i, j, k)) / gridSizeK[k + 1]
          + 0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) * currGC(W, i, j, k) * (gridInit(THETA, i, j, k) - gridInit(THETA, i, j, k - 1)) / gridSizeK[k]) // -w/p*dpT/dz (mean state)
          + Kx / powf(gridSizeI, 2) * (prevGC(THETA, i + 1, j, k) - 2.0f * prevGC(THETA, i, j, k) + prevGC(THETA, i - 1, j, k)) // Diffusion (implicit)
          + Ky / powf(gridSizeJ, 2) * (prevGC(THETA, i, j + 1, k) - 2.0f * prevGC(THETA, i, j, k) + prevGC(THETA, i, j - 1, k)) // Diffusion (implicit)
          + Kz / powf(gridSizeK[k], 2) * (prevGC(THETA, i, j, k + 1) - 2.0f * prevGC(THETA, i, j, k) + prevGC(THETA, i, j, k - 1)); // d2T/dx2+d2T/dz2

        gridRslow(Pi, i, j, k) =
          -1.0f * (powf(cmax, 2) / (currGC(RO, i, j, k) * cpd * powf(gridInit(THETA, i, j, k) + 0.61f * gridInit(QV, i, j, k), 2))) * (// multiplier -cs^2/(cpd* p*T^2)
          +(currGC(RO, i, j, k) * (gridInit(THETA, i, j, k) + 0.61f * gridInit(QV, i, j, k)) * (currGC(U, i + 1, j, k) - currGC(U, i, j, k))) / gridSizeI // pTdu/dx

          + (currGC(RO, i, j, k) * (gridInit(THETA, i, j, k) + 0.61f * gridInit(QV, i, j, k)) * (currGC(V, i, j + 1, k) - currGC(V, i, j, k))) / gridSizeJ // pTdv/dx

          + (0.5f * (currGC(RO, i, j, k + 1) + currGC(RO, i, j, k)) * currGC(W, i, j, k + 1) * 0.5f * (gridInit(THETA, i, j, k + 1) + gridInit(THETA, i, j, k))
          - 0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) * currGC(W, i, j, k) * 0.5f * (gridInit(THETA, i, j, k) + gridInit(THETA, i, j, k - 1))) / gridSizeK[k] // pTdw/dz
          )
          + Kx / powf(gridSizeI, 2) * (prevGC(Pi, i + 1, j, k) - 2.0f * prevGC(Pi, i, j, k) + prevGC(Pi, i - 1, j, k)) // Diffusion (implicit)
          + Ky / powf(gridSizeI, 2) * (prevGC(Pi, i, j + 1, k) - 2.0f * prevGC(Pi, i, j, k) + prevGC(Pi, i, j - 1, k)) // Diffusion (implicit)
          + Kz / powf(gridSizeK[k], 2) * (prevGC(Pi, i, j, k + 1) - 2.0f * prevGC(Pi, i, j, k) + prevGC(Pi, i, j, k - 1)); // d2P/dx2+d2P/dz2

        // Moisture terms
        gridRslow(QV, i, j, k) =
          -1.0f / gridSizeI * (currGC(U, i + 1, j, k) * 0.5f * (currGC(QV, i + 1, j, k) + currGC(QV, i, j, k))
          - currGC(U, i, j, k) * 0.5f * (currGC(QV, i, j, k) + currGC(QV, i - 1, j, k))) // -duqv/dx

          - 1.0f / gridSizeJ * (currGC(V, i, j + 1, k) * 0.5f * (currGC(QV, i, j + 1, k) + currGC(QV, i, j, k))
          - currGC(V, i, j, k) * 0.5f * (currGC(QV, i, j, k) + currGC(QV, i, j - 1, k))) // -dvqv/dy

          - 1.0f / (currGC(RO, i, j, k) * gridSizeK[k]) * (
          0.5f * (currGC(RO, i, j, k + 1) + currGC(RO, i, j, k)) * currGC(W, i, j, k + 1) * 0.5f * (currGC(QV, i, j, k + 1) + currGC(QV, i, j, k))
          - 0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) * currGC(W, i, j, k) * 0.5f * (currGC(QV, i, j, k) + currGC(QV, i, j, k - 1))) // -dpwqv/dz
          - 1.0f / (currGC(RO, i, j, k)) * 0.5f * (
          0.5f * (currGC(RO, i, j, k + 1) + currGC(RO, i, j, k)) * currGC(W, i, j, k + 1) * (gridInit(QV, i, j, k + 1) - gridInit(QV, i, j, k)) / gridSizeK[k + 1]
          + 0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) * currGC(W, i, j, k) * (gridInit(QV, i, j, k) - gridInit(QV, i, j, k - 1)) / gridSizeK[k]) // -w/p*dpqv/dz (mean state)
          + Kx / powf(gridSizeI, 2) * (prevGC(QV, i + 1, j, k) - 2.0f * prevGC(QV, i, j, k) + prevGC(QV, i - 1, j, k)) // Diffusion (implicit)
          + Ky / powf(gridSizeI, 2) * (prevGC(QV, i, j + 1, k) - 2.0f * prevGC(QV, i, j, k) + prevGC(QV, i, j - 1, k)) // Diffusion (implicit)
          + Kz / powf(gridSizeK[k], 2) * (prevGC(QV, i, j, k + 1) - 2.0f * prevGC(QV, i, j, k) + prevGC(QV, i, j, k - 1)); // d2q/dx2+d2q/dz2

        gridRslow(QC, i, j, k) =
          -1.0f / gridSizeI * (currGC(U, i + 1, j, k) * 0.5f * (currGC(QC, i + 1, j, k) + currGC(QC, i, j, k))
          - currGC(U, i, j, k) * 0.5f * (currGC(QC, i, j, k) + currGC(QC, i - 1, j, k))) // -duqv/dx

          - 1.0f / gridSizeJ * (currGC(V, i, j + 1, k) * 0.5f * (currGC(QC, i, j + 1, k) + currGC(QC, i, j, k))
          - currGC(V, i, j, k) * 0.5f * (currGC(QC, i, j, k) + currGC(QC, i, j - 1, k))) // -duqv/dx

          - 1.0f / (currGC(RO, i, j, k) * gridSizeK[k]) * (
          0.5f * (currGC(RO, i, j, k + 1) + currGC(RO, i, j, k)) * currGC(W, i, j, k + 1) * 0.5f * (currGC(QC, i, j, k + 1) + currGC(QC, i, j, k))
          - 0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) * currGC(W, i, j, k) * 0.5f * (currGC(QC, i, j, k) + currGC(QC, i, j, k - 1))) // -dpwqv/dz
          + Kx / powf(gridSizeI, 2) * (prevGC(QC, i + 1, j, k) - 2.0f * prevGC(QC, i, j, k) + prevGC(QC, i - 1, j, k)) // Diffusion (implicit)
          + Ky / powf(gridSizeI, 2) * (prevGC(QC, i, j + 1, k) - 2.0f * prevGC(QC, i, j, k) + prevGC(QC, i, j - 1, k)) // Diffusion (implicit)
          + Kz / powf(gridSizeK[k], 2) * (prevGC(QC, i, j, k + 1) - 2.0f * prevGC(QC, i, j, k) + prevGC(QC, i, j, k - 1)); // d2q/dx2+d2q/dz2

        gridRslow(QR, i, j, k) =
          -1.0f / gridSizeI * (currGC(U, i + 1, j, k) * 0.5f * (currGC(QR, i + 1, j, k) + currGC(QR, i, j, k))
          - currGC(U, i, j, k) * 0.5f * (currGC(QR, i, j, k) + currGC(QR, i - 1, j, k))) // -duqv/dx

          - 1.0f / gridSizeJ * (currGC(V, i, j + 1, k) * 0.5f * (currGC(QR, i, j + 1, k) + currGC(QR, i, j, k))
          - currGC(V, i, j, k) * 0.5f * (currGC(QR, i, j, k) + currGC(QR, i, j - 1, k))) // -dvqv/dy

          - 1.0f / (currGC(RO, i, j, k) * gridSizeK[k]) * (
          0.5f * (currGC(RO, i, j, k + 1) + currGC(RO, i, j, k)) * currGC(W, i, j, k + 1) * 0.5f * (currGC(QR, i, j, k + 1) + currGC(QR, i, j, k))
          - 0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) * currGC(W, i, j, k) * 0.5f * (currGC(QR, i, j, k) + currGC(QR, i, j, k - 1))) // -dpwqv/dz
          + Kx / powf(gridSizeI, 2) * (prevGC(QR, i + 1, j, k) - 2.0f * prevGC(QR, i, j, k) + prevGC(QR, i - 1, j, k)) // Diffusion (implicit)
          + Ky / powf(gridSizeI, 2) * (prevGC(QR, i, j + 1, k) - 2.0f * prevGC(QR, i, j, k) + prevGC(QR, i, j - 1, k)) // Diffusion (implicit)
          + Kz / powf(gridSizeK[k], 2) * (prevGC(QR, i, j, k + 1) - 2.0f * prevGC(QR, i, j, k) + prevGC(QR, i, j, k - 1)); // d2q/dx2+d2q/dz2

        gridRslow(RO, i, j, k) = 0.0f;
      }
    }
  }
} //

/* STEP2: Kelsner Microphysics */
void simulateSTEP2(
  const float dT, const int gridX, const int gridY, const int gridZ, const float simulationTime,
  const float gridSizeI, const float gridSizeJ, float *gridSizeK,
  Grid3D& prevGC, Grid3D& currGC, Grid3D& nextGC,
  Grid3D& gridRslow, Grid3D& gridInit, RadStruct& st) {

  for (int k = 1; k < gridZ - 1; k++) {
    for (int j = 0; j < gridY; j++) {
      for (int i = 0; i < gridX; i++) {

        // Kessler microphysics.
        // A = max[ k1 * (qc-qc0) , 0 ] : autoconverstion qc -> qr
        // B = k2 * qc * qr^7/8	: accretion qc -> qr
        // C: condensation ; qv <-> qv
        // E: evaporation ; qr -> qv
        // All values from t-1 step
        // Order of calculation matters here

        float A_conv = 0.0;
        if (prevGC(QC, i, j, k) > 0.001) A_conv = std::max<float>(0.0f, 0.001* (prevGC(QC, i, j, k) - 0.001)); // conversion cloud -> rain

        float B_acc = std::max<float>(0.0f, gridInit(RO, i, j, k) * 2.2f * prevGC(QC, i, j, k) * powf(prevGC(QR, i, j, k), 0.875f)); // accretion cloud -> rain

        A_conv *= st.rainProbability;
        B_acc *= st.rainProbability;

        // Saturation adjustment (Soong & Ogura)
        float pmean = powf(gridInit(Pi, i, j, k), cpd / Rd) *p_0; // Mean pressure
        float qvs = (380.0f / pmean) * exp(7.5f *log(10.0f) *
          ((prevGC(THETA, i, j, k) + gridInit(THETA, i, j, k)) * ((prevGC(Pi, i, j, k) + gridInit(Pi, i, j, k))) - 273.0f) /
          ((prevGC(THETA, i, j, k) + gridInit(THETA, i, j, k)) * ((prevGC(Pi, i, j, k) + gridInit(Pi, i, j, k))) - 36.0f)); // Saturation mixing ratio
        prevGC(QV, i, j, k) = std::max<float>(prevGC(QV, i, j, k), -1.0f * gridInit(QV, i, j, k)); // remove negative values
        float rsub = qvs * (7.5f * logf(10.0f) * (273.0f - 36.0f) *Llv / cpd) /
          powf(gridInit(Pi, i, j, k) * (prevGC(THETA, i, j, k) + gridInit(THETA, i, j, k)), 2);

        float Cond = std::min<float>(prevGC(QV, i, j, k) + gridInit(QV, i, j, k),
          std::max<float>(0.0f, ((prevGC(QV, i, j, k) + gridInit(QV, i, j, k)) - qvs) / (1.0f + rsub))); // Condensation (qv -> qc)

        float Cvent = 1.6f + 124.9f * powf(gridInit(RO, i, j, k) * prevGC(QC, i, j, k), 0.2046f); // ventillation factor
        float Evap = std::min<float>(std::min<float>(prevGC(QR, i, j, k), std::max<float>(-1.0f *Cond - prevGC(QC, i, j, k), 0.0f)), // 3 options
          dT * Cvent * powf(gridInit(RO, i, j, k) * prevGC(QR, i, j, k), 0.525f) / (5.4e5 + 2.55e8 / (pmean*qvs))
          *std::max<float>(qvs - prevGC(QV, i, j, k), 0.0f) / (gridInit(RO, i, j, k) *qvs));
        Cond = std::max<float>(Cond, -1.0f * prevGC(QC, i, j, k));

        gridRslow(QV, i, j, k) = gridRslow(QV, i, j, k) - Cond + Evap; // Net mass conversion

        gridRslow(QC, i, j, k) = gridRslow(QC, i, j, k) + Cond - A_conv - B_acc; // Net mass conversion

        float vterm0 = 36.34f*sqrtf(gridInit(RO, i, j, 0) / gridInit(RO, i, j, k)) * powf(std::max<float>(gridInit(RO, i, j, k) * prevGC(QR, i, j, k), 0.0f), 0.1364f);
        float vterm1 = 36.34f*sqrtf(gridInit(RO, i, j, 0) / gridInit(RO, i, j, k + 1)) * powf(std::max<float>(gridInit(RO, i, j, k + 1) * prevGC(QR, i, j, k + 1), 0.0f), 0.1364f); // vT terminal velocity
        // note, it's possible that vT > CFL.

        gridRslow(QR, i, j, k) = gridRslow(QR, i, j, k) + A_conv + B_acc - Evap // Net mass change
          + 1.0f / (currGC(RO, i, j, k) * gridSizeK[k]) * (
          0.5f * (currGC(RO, i, j, k + 1) + currGC(RO, i, j, k)) *vterm1* 0.5f * (prevGC(QR, i, j, k + 1) + prevGC(QR, i, j, k))
          - 0.5f * (currGC(RO, i, j, k) + currGC(RO, i, j, k - 1)) *vterm0* 0.5f * (prevGC(QR, i, j, k) + prevGC(QR, i, j, k - 1))); // Falling rain

        gridRslow(THETA, i, j, k) = gridRslow(THETA, i, j, k) + Llv / (cpd * gridInit(Pi, i, j, k)) * (Cond - Evap); // latent heating Lv/(cpd * P) * (C-E);
      }
    }
  }
}

/* STEP3: Move forward in time */
void simulateSTEP3(
  const float dT, const int gridX, const int gridY, const int gridZ, const float simulationTime,
  const float gridSizeI, const float gridSizeJ, float *gridSizeK,
  Grid3D& prevGC, Grid3D& currGC, Grid3D& nextGC,
  Grid3D& gridRslow, Grid3D& gridInit) {

  for (int k = 1; k < gridZ - 1; k++) {
    for (int j = 0; j < gridY; j++) {
      for (int i = 0; i < gridX; i++) {

        if (simulationTime == 0.0f) { // 1st step: forward in time
          //	printf("first Iteration\n");
          nextGC(U, i, j, k) = currGC(U, i, j, k) + dT * gridRslow(U, i, j, k);
          nextGC(V, i, j, k) = currGC(V, i, j, k) + dT * gridRslow(V, i, j, k);
          nextGC(W, i, j, k) = currGC(W, i, j, k) + dT * gridRslow(W, i, j, k);
          if ((k < 2)) nextGC(W, i, j, k) = 0.0f; // top & bottom BCs //|| (k==zEnd)
          nextGC(Pi, i, j, k) = currGC(Pi, i, j, k) + dT * gridRslow(Pi, i, j, k);
          nextGC(THETA, i, j, k) = currGC(THETA, i, j, k) + dT * gridRslow(THETA, i, j, k);
          nextGC(QV, i, j, k) = currGC(QV, i, j, k) + dT * gridRslow(QV, i, j, k);
          nextGC(QC, i, j, k) = currGC(QC, i, j, k) + dT * gridRslow(QC, i, j, k);
          nextGC(QR, i, j, k) = currGC(QR, i, j, k) + dT * gridRslow(QR, i, j, k);
          nextGC(RO, i, j, k) = currGC(RO, i, j, k) + dT * gridRslow(RO, i, j, k);

        } else { // subsequent steps: leapfrog

          //	printf("No first Iteration\n");
          nextGC(U, i, j, k) = prevGC(U, i, j, k) + 2.0f * dT * gridRslow(U, i, j, k);
          nextGC(V, i, j, k) = prevGC(V, i, j, k) + 2.0f * dT * gridRslow(V, i, j, k);
          nextGC(W, i, j, k) = prevGC(W, i, j, k) + 2.0f * dT * gridRslow(W, i, j, k);
          if ((k < 2)) nextGC(W, i, j, k) = 0.0f; // top & bottom BCs // || (k==zEnd)
          nextGC(Pi, i, j, k) = prevGC(Pi, i, j, k) + 2.0f * dT * gridRslow(Pi, i, j, k);
          nextGC(THETA, i, j, k) = prevGC(THETA, i, j, k) + 2.0f * dT * gridRslow(THETA, i, j, k);
          nextGC(QV, i, j, k) = prevGC(QV, i, j, k) + 2.0f * dT * gridRslow(QV, i, j, k);
          nextGC(QC, i, j, k) = prevGC(QC, i, j, k) + 2.0f * dT * gridRslow(QC, i, j, k);
          nextGC(QR, i, j, k) = prevGC(QR, i, j, k) + 2.0f * dT * gridRslow(QR, i, j, k);
          nextGC(RO, i, j, k) = prevGC(RO, i, j, k) + 2.0f * dT * gridRslow(RO, i, j, k);

          // Roberts-Asselin filter
          currGC(U, i, j, k) = currGC(U, i, j, k) + 0.1f * (nextGC(U, i, j, k) - 2.0f * currGC(U, i, j, k) + prevGC(U, i, j, k));
          currGC(V, i, j, k) = currGC(V, i, j, k) + 0.1f * (nextGC(V, i, j, k) - 2.0f * currGC(V, i, j, k) + prevGC(V, i, j, k));
          currGC(W, i, j, k) = currGC(W, i, j, k) + 0.1f * (nextGC(W, i, j, k) - 2.0f * currGC(W, i, j, k) + prevGC(W, i, j, k));
          currGC(THETA, i, j, k) = currGC(THETA, i, j, k) + 0.1f * (nextGC(THETA, i, j, k) - 2.0f * currGC(THETA, i, j, k) + prevGC(THETA, i, j, k));
          currGC(Pi, i, j, k) = currGC(Pi, i, j, k) + 0.1f * (nextGC(Pi, i, j, k) - 2.0f * currGC(Pi, i, j, k) + prevGC(Pi, i, j, k));
          currGC(QV, i, j, k) = currGC(QV, i, j, k) + 0.1f * (nextGC(QV, i, j, k) - 2.0f * currGC(QV, i, j, k) + prevGC(QV, i, j, k));
          currGC(QC, i, j, k) = currGC(QC, i, j, k) + 0.1f * (nextGC(QC, i, j, k) - 2.0f * currGC(QC, i, j, k) + prevGC(QC, i, j, k));
          currGC(QR, i, j, k) = currGC(QR, i, j, k) + 0.1f * (nextGC(QR, i, j, k) - 2.0f * currGC(QR, i, j, k) + prevGC(QR, i, j, k));
          currGC(RO, i, j, k) = currGC(RO, i, j, k) + 0.1f * (nextGC(RO, i, j, k) - 2.0f * currGC(RO, i, j, k) + prevGC(RO, i, j, k));
        }
      }
    }
  }
}// STEP 3

/* STEP4: Radiation model */
void simulateSTEP4(
  const float dT, const int gridX, const int gridY, const int gridZ, const float simulationTime,
  const float gridSizeI, const float gridSizeJ, float *gridSizeK,
  Grid3D& prevGC, Grid3D& currGC, Grid3D& nextGC,
  Grid3D& gridRslow, Grid3D& gridInit, Ground3D& ground, RadStruct& st) {

  for (int j = 0; j < gridY; j++) {
    for (int i = 0; i < gridX; i++) {

      const float T_M = 29.0f + 273.15f;// Invariable slab temperature //INIT 10.0f 32.0f
      const float dur = 3600.0f * 24.0f;// * 5.0f;//24h
      const float S_const = -1.127f;//Solar constant km/s

      ////////////////////////////////////////
      // INIT VALUES
      if (simulationTime == 0.0f) { // 1st step: forward in time 
        currGC(THETA, i, j, 0) = 0;

        ground(GR_TG, i, j) = 23.5f + 273.15f;
        ground(GR_TA, i, j) = gridInit(THETA, i, j, 0);

        ground(GR_TG_RESET, i, j) = FLT_MAX;// INF

        ground(GR_TG_CORR, i, j) = 0.0f;
        ground(GR_TA_CORR, i, j) = 0.0f;

        ground(GR_CLOUD_COVER, i, j) = 0.0f;
      }

      ////////////////////////////////////////
      // CLOUD COVERAGE

      if ((int(simulationTime / dT) % (60 * 5)) == 0) {  // each 300 steps
        float cloudTotal = 0.0f;
        for (int z = static_cast<int>(0.33f * gridZ); z < static_cast<int>(0.83f * gridZ); z++) {
          float density = nextGC(QC, i, j, z);
          if (density == 0.0f)
            continue;
          if (density > 2e-3f) {
            density = 0.99f;
          } else {
            if (density < 1e-3f) {
              density = 0.0f;
            } else {
              density = -1520000.0f * (density*density) + 5360.00f * (density) -3.74f;
            }
          }
          cloudTotal += density* 0.1f;  //0.05f so it has soft borders
          if (cloudTotal >= 1.0f) {
            break; // not necessary to check more
          }
        }
        ground(GR_CLOUD_COVER, i, j) = fmin(cloudTotal, 1.0f);
      }


      ////////////////////////////////////////
      // UTC
      float t_UTC = st.initTimeUTC_hours + (simulationTime / 3600.0f);//day overflow
      int advancedDays = int(t_UTC / (24.0f));//full days
      float  dayInYearUTC = st.initDayInYearUTC + advancedDays;
      while (dayInYearUTC > 365)
        dayInYearUTC -= 365;
      t_UTC -= advancedDays* 24.0f;

      ////////////////////////////////////////
      // LOCAL
      float t_Local = t_UTC + st.timeZone;
      if (t_Local < 0.0f) t_Local += 24.0f;
      if (t_Local > 24.0f) t_Local -= 24.0f;


      float lat = st.latitudeRad;
      float longi = -st.longitudeRad;//note NEGATE (West)

      float delta = 0.409f*cos((2.0f * M_PI) * (dayInYearUTC - 173.0f) / (365.25f));//d_s: solarDeclineAngle
      float sinPSI = sin(lat) *sin(delta) - cos(lat) *cos(delta) *cos(((M_PI*t_UTC) / 12.0f) - longi);

      float gamma = 0.0000010717f * powf(t_Local, 5.0f) + 0.0000818369f * powf(t_Local, 4.0f) - 0.0060500842f * powf(t_Local, 3.0f) + 0.0772306397f * powf(t_Local, 2.0f) + 0.1444444444f*t_Local - 1.8441558442f;

      ////////////////////////////////////////
      // RADITATION

      float alb = ground(GR_ALBEDO, i, j);
      float c_g_a = ground(GR_CGA, i, j);

      float sig_l = ground(GR_CLOUD_COVER, i, j);
      float sig_m = ground(GR_CLOUD_COVER, i, j);
      const float sig_h = 0.1f;

      float I = 0.08f* (1.0f - 0.1f *sig_h - 0.3f *sig_m - 0.6f *sig_l);

      float Tk = (0.6f + 0.2f *sinPSI) * (1.0f - 0.4f *sig_h) * (1.0f - 0.7f *sig_m) * (1.0f - 0.4f *sig_l);  //trans
      float Q_net = (1.0f - alb) * S_const * Tk *sinPSI + I;
      if (sinPSI < 0)
        Q_net = I;
      float a_fr;
      if (ground(GR_TG, i, j) > ground(GR_TA, i, j)) {
        //DAY
        a_fr = 3e-4f;
      } else {
        //NIGHT
        a_fr = 1.1e-4f;
      }

      float T_G_t = ((-Q_net / c_g_a) + (2.0f * M_PI / dur* (T_M - ground(GR_TG, i, j))) - (a_fr* (ground(GR_TG, i, j) - ground(GR_TA, i, j))));
      float Q_g = -1.0f * ((c_g_a * T_G_t) + (2.0f * M_PI*c_g_a / dur* (ground(GR_TG, i, j) - T_M)));  //Units are fomd
      float  Q_h = (-Q_net + Q_g) / ground(GR_BETA_INV, i, j);

      ground(GR_TG, i, j) += (dT *T_G_t) + ground(GR_TG_CORR, i, j);// NEW TG
      ground(GR_TA, i, j) += (dT * Q_h * 1.0e-3f) + ground(GR_TA_CORR, i, j); // Introduced this new time parameterization //NEW TA

      ///////////////////////////////////
      // STEP 0: Save ref value after 2 hours of simulation
      if ((ground(GR_TG_RESET, i, j) == FLT_MAX) && (simulationTime >= 3600.0f * 2.0f)) {  // put first FLT_MAX to avoid to comparisons
        ground(GR_TG_RESET, i, j) = ground(GR_TG, i, j);
        ground(GR_TA_RESET, i, j) = ground(GR_TA, i, j);
        if (i == 0 && j == 0) {
          printf("** Save Ref: %f (%.2f)\n", simulationTime, simulationTime / 3600.0f);
        }
      }

      ///////////////////////////////////
      // STEP1: Update Correction after Each 24hours (+2h)
      if ((simulationTime >= 3600.0f * (2.0f + 24.0f)) && ((int(simulationTime) - 2 * 3600)) % (24 * 3600) == 0) { // RESET
        if (i == 0) {
          printf("** Reset: %f (%f)\n", simulationTime, simulationTime / 3600.0f);
        }
        float TG_diff = ground(GR_TG_RESET, i, j) - ground(GR_TG, i, j);
        float TA_diff = ground(GR_TA_RESET, i, j) - ground(GR_TA, i, j);
        ground(GR_TG_CORR, i, j) = (TG_diff / (24.0f * 3600.0f)) * dT * 1.2f; //1.2f correction factor
        ground(GR_TA_CORR, i, j) = (TA_diff / (24.0f * 3600.0f)) * dT * 1.2f;
      }

      nextGC(THETA, i, j, 0) = ground(GR_TA, i, j) + gamma * gridSizeK[0] / 100.0f - gridInit(THETA, i, j, 0);//transfer of Ta to THETA
    }
  }
}//STEP 4


///////////////////////////////////////////////////////////////////////////
// INITIALIZATION

void initSimulation(const float _dT, const int _gridX, const int _gridY, const int _gridZ, const float _gridSizeI, const float _gridSizeJ, std::vector<float>& _gridSizeK,
  Grid3D& _grid0Var, Grid3D& _gridInitVar,
  Ground3D& _ground, RadStruct& _radStruct) {

  dT = _dT;
  gridX = _gridX;
  gridY = _gridY;
  gridZ = _gridZ;
  gridSizeI = _gridSizeI;
  gridSizeJ = _gridSizeJ;
  ground = _ground;
  radStruct = _radStruct;

  gridSizeK = _gridSizeK;

  // Arrays.
  for (int i = 0; i < 3; i++) {
    grid3D[i] = _grid0Var;
  }
  gridRslow = _grid0Var;
  gridInit = _gridInitVar;
  printf("<<initSimulation\n");

  firstTime = false;
}//



///////////////////////////////////////////////////////////////////////////
// RUN SIMULATION

void simulateStep(const int numSteps, float& simulationTime) {
  // Check whether it was initialized
  if (firstTime == true) {
    printf("ERROR: System should be initialized first.\n");
    return;
  }

  // Run sumulation.
  const bool check = true;
  for (int nS = 0; nS < numSteps; nS++) {
    int nextInd = (currInd + 1) % 3;  //3 number of time array
    int prevInd = (currInd - 1);
    if (prevInd < 0) prevInd = 2;  // Set the last step.
    // printf("Step %d: prev %d curr %d next %d\n", nS, prevInd, currInd, nextInd);

    for (int i = 0; i < NUM_ELEM; i++) {
      memset(&gridRslow.var[i][0], 0, size_t(sizeof(float) * gridX * gridY * gridZ));
    }

    // Fundamental equations.
    simulateSTEP1(
      dT, gridX, gridY, gridZ, simulationTime,
      gridSizeI, gridSizeJ, gridSizeK.data(),
      grid3D[prevInd], grid3D[currInd], grid3D[nextInd],
      gridRslow, gridInit);

    // Microphysics.
    simulateSTEP2(
      dT, gridX, gridY, gridZ, simulationTime,
      gridSizeI, gridSizeJ, gridSizeK.data(),
      grid3D[prevInd], grid3D[currInd], grid3D[nextInd],
      gridRslow, gridInit, radStruct);

    // Move in time.
    simulateSTEP3(
      dT, gridX, gridY, gridZ, simulationTime,
      gridSizeI, gridSizeJ, gridSizeK.data(),
      grid3D[prevInd], grid3D[currInd], grid3D[nextInd],
      gridRslow, gridInit);

    // Radiation.
    simulateSTEP4(
      dT, gridX, gridY, gridZ, simulationTime,
      gridSizeI, gridSizeJ, gridSizeK.data(),
      grid3D[prevInd], grid3D[currInd], grid3D[nextInd],
      gridRslow, gridInit, ground, radStruct);

    currInd = (currInd + 1) % 3;  // 3 number of time array

    simulationTime += dT;
  }
}//