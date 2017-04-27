/*
mVMC - A numerical solver package for a wide range of quantum lattice models based on many-variable Variational Monte Carlo method
Copyright (C) 2016 Takahiro Misawa, Satoshi Morita, Takahiro Ohgoe, Kota Ido, Mitsuaki Kawamura, Takeo Kato, Masatoshi Imada.

This program is developed based on the mVMC-mini program
(https://github.com/fiber-miniapp/mVMC-mini)
which follows "The BSD 3-Clause License".

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details. 

You should have received a copy of the GNU General Public License 
along with this program. If not, see http://www.gnu.org/licenses/. 
*/
/*-------------------------------------------------------------
 * Variational Monte Carlo
 * Read Definition Files
 *-------------------------------------------------------------
 * by Satoshi Morita
 *-------------------------------------------------------------*/

#include <ctype.h>
#include <stdlib.h>
#include "./include/readdef.h"
#include "./include/global.h"
#include "safempi_fcmp.c"


#define _NOTBACKFLOW

int ReadDefFileError(const char *defname);
int ReadDefFileNInt(char *xNameListFile, MPI_Comm comm);
int ReadDefFileIdxPara(char *xNameListFile, MPI_Comm comm);


int CheckSite(const int iSite, const int iMaxNum);
int CheckPairSite(const int iSite1, const int iSite2, const int iMaxNum);
int CheckQuadSite(const int iSite1, const int iSite2, const int iSite3, const int iSite4, const int iMaxNum);

int ReadDefFileError(const char *defname){
  fprintf(stderr, "error: %s (Broken file or Not exist)\n", defname);
  return 1;
}

int ReadGreen(char *xNameListFile, int Nca, int**caIdx, int Ncacadc, int**cacaDCIdx, int Ns){
  FILE *fp;
  char defname[D_FileNameMax];
  char ctmp[D_FileNameMax], ctmp2[256];
  int itmp;
  int iKWidx=0;
  char *cerr;

  int i,j,n,idx,idx0,idx1,info=0;
  int fidx=0; /* index for OptFlag */
  int count_idx=0;
  int x0,x1,x2,x3,x4,x5,x6,x7;

  cFileNameListFile = malloc(sizeof(char)*D_CharTmpReadDef*KWIdxInt_end);
  fprintf(stdout, "  Read File %s .\n", xNameListFile);
  if(GetFileName(xNameListFile, cFileNameListFile)!=0){
    fprintf(stderr, "  error: Definition files(*.def) are incomplete.\n");
    return -1;
  }

  for(iKWidx=KWLocSpin; iKWidx< KWIdxInt_end; iKWidx++){
    strcpy(defname, cFileNameListFile[iKWidx]);

    if(strcmp(defname,"")==0) continue;

    fp = fopen(defname, "r");
    if(fp==NULL){
      info= ReadDefFileError(defname);
      fclose(fp);
      continue;
    }

    /*=======================================================================*/
    for (i = 0; i < IgnoreLinesInDef; i++)
      cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
    idx=0;
    idx0=0;
    switch(iKWidx){
      case KWOneBodyG:
        /*cisajs.def----------------------------------------*/
        if(Nca>0){
          idx = 0;
          while( fscanf(fp, "%d %d %d %d\n",
                        &(x0), &(x1), &(x2), &(x3)) != EOF){
            caIdx[idx][0] = x0;
            caIdx[idx][1] = x1;
            caIdx[idx][2] = x2;
            caIdx[idx][3] = x3;
            if(CheckPairSite(x0, x2, Ns) != 0){
              fprintf(stderr, "Error: Site index is incorrect. \n");
              return(-1);
            }

            if(x1 != x3){
              fprintf(stderr, "  Error:  Sz non-conserved system is not yet supported in mVMC ver.1.0.\n");
              return(-1);
            }
            idx++;
          }
          if(idx!=Nca) info=ReadDefFileError(defname);
        }
        fclose(fp);
        break;
      case KWTwoBodyG:
        /*cisajscktaltdc.def--------------------------------*/
        if(Ncacadc>0){
          idx = 0;
          while( fscanf(fp, "%d %d %d %d %d %d %d %d\n",
                        &(x0), &(x1), &(x2), &(x3), &(x4),
                        &(x5), &(x6), &(x7) ) != EOF ){
            cacaDCIdx[idx][0] = x0;
            cacaDCIdx[idx][1] = x1;
            cacaDCIdx[idx][2] = x2;
            cacaDCIdx[idx][3] = x3;
            cacaDCIdx[idx][4] = x4;
            cacaDCIdx[idx][5] = x5;
            cacaDCIdx[idx][6] = x6;
            cacaDCIdx[idx][7] = x7;
            idx++;
            if(CheckQuadSite(x0, x2, x4, x6, Ns) != 0){
              fprintf(stderr, "Error: Site index is incorrect. \n");
              return(-1);
            }

            if(x1 != x3 || x5 != x7){
              fprintf(stderr, "  Error:  Sz non-conserved system is not yet supported in mVMC ver.1.0.\n");
              return(-1);
            }
          }
          if(idx!=Ncacadc) info=ReadDefFileError(defname);
        }
        fclose(fp);
        break;
      default:
        fclose(fp);
        break;
    }
  }
  return 0;
}

///
/// \param xNameListFile FileNameLists
/// \param Nca Number of CisAjs
/// \param Ncacadc Number of CisAjsCktAltDC
/// \param Ns Number of sites
/// \return Number of calculation target
int CountOneBodyGForLanczos(char *xNameListFile, int Nca, int Ncacadc, int Ns, int **caIdx, int **iFlgOneBodyG){

  int info=0;
  int i, j, isite1, isite2;
  int *pInt;
  int icount=0;
  int NFullOneBodyG=Ns*2*Ns*2;
  int **cacaDCIdx;

  cacaDCIdx = malloc(sizeof(int*)*Ncacadc);
  //pInt=cacaDCIdx[0];
  for(i=0;i<Ncacadc;i++) {
    cacaDCIdx[i] = malloc(sizeof(int)*8);
  }

  for(i=0; i<2*Ns; i++) {
    for (j = 0; j < 2 * Ns; j++) {
      iFlgOneBodyG[i][j] = -1;
    }
  }
  info=ReadGreen(xNameListFile, Nca, caIdx,  Ncacadc, cacaDCIdx, Ns);
  if( info !=0){
      return (info);
  }

  for(i=0; i<Nca; i++){
    isite1=caIdx[i][0]+caIdx[i][1]*Ns;
    isite2=caIdx[i][2]+caIdx[i][3]*Ns;
    if(iFlgOneBodyG[isite1][isite2]==-1){
      iFlgOneBodyG[isite1][isite2]=icount;
      icount++;
    }
  }
  //cisajscktalt -> cisajs, cktalt
  for(i=0; i<Ncacadc; i++){
    isite1=cacaDCIdx[i][0]+cacaDCIdx[i][1]*Ns;
    isite2=cacaDCIdx[i][2]+cacaDCIdx[i][3]*Ns;
    if(iFlgOneBodyG[isite1][isite2]==-1){
      iFlgOneBodyG[isite1][isite2]=icount;
      icount++;
    }
    isite1=cacaDCIdx[i][4]+cacaDCIdx[i][5]*Ns;
    isite2=cacaDCIdx[i][6]+cacaDCIdx[i][7]*Ns;
    if(iFlgOneBodyG[isite1][isite2]==-1){
      iFlgOneBodyG[isite1][isite2]=icount;
      icount++;
    }
  }
  free(cacaDCIdx);
  return icount;
}

int ReadDefFileNInt(char *xNameListFile, MPI_Comm comm) {
  FILE *fp;
  char defname[D_FileNameMax];
  char ctmp[D_FileNameMax];
  char ctmp2[D_FileNameMax];

  int itmp;
  char *cerr;

  int rank, info = 0;
  const int nBufInt = ParamIdxInt_End;
  const int nBufDouble = ParamIdxDouble_End;
  const int nBufChar = D_FileNameMax;
  int bufInt[nBufInt];
  double bufDouble[nBufDouble];
  int iKWidx = 0;
  int iFlgOrbitalSimple = 0;
  int iOrbitalComplex = 0;
  iFlgOrbitalGeneral = 0;
  MPI_Comm_rank(comm, &rank);

  if (rank == 0) {
    cFileNameListFile = malloc(sizeof(char) * D_CharTmpReadDef * KWIdxInt_end);
    fprintf(stdout, "  Read File %s .\n", xNameListFile);
    if (GetFileName(xNameListFile, cFileNameListFile) != 0) {
      fprintf(stderr, "  error: Definition files(*.def) are incomplete.\n");
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    for (iKWidx = 0; iKWidx < KWIdxInt_end; iKWidx++) {
      strcpy(defname, cFileNameListFile[iKWidx]);
      if (strcmp(defname, "") == 0) {
        switch (iKWidx) {
          case KWModPara:
          case KWLocSpin:
            fprintf(stderr, "  Error: Need to make a def file for %s.\n", cKWListOfFileNameList[iKWidx]);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            break;
          default:
            break;
        }
      }
    }

    for (iKWidx = 0; iKWidx < KWIdxInt_end; iKWidx++) {
      strcpy(defname, cFileNameListFile[iKWidx]);
      if (strcmp(defname, "") == 0) continue;
      fprintf(stdout, "  Read File '%s' for %s.\n", defname, cKWListOfFileNameList[iKWidx]);
      fp = fopen(defname, "r");
      if (fp == NULL) {
        info = ReadDefFileError(defname);
        fclose(fp);
        break;
      } else {

        switch (iKWidx) {
          case KWModPara:
            /* Read modpara.def---------------------------------------*/
            //TODO: add error procedure here when parameters are not enough.
            SetDefultValuesModPara(bufInt, bufDouble);
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &itmp); //2
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp); //3
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp); //4
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp); //5
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %s\n", ctmp, CDataFileHead); //6
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sprintf(ctmp, "output/%s", CDataFileHead);
            strcpy(CDataFileHead, ctmp);
            sscanf(ctmp2, "%s %s\n", ctmp, CParaFileHead); //7
            sprintf(ctmp, "output/%s", CParaFileHead);
            strcpy(CParaFileHead, ctmp);
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);   //8
            info = system("mkdir -p output");

            double dtmp;
            while (fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp) != NULL) {
              if (*ctmp2 == '\n' || ctmp2[0] == '-') continue;
              sscanf(ctmp2, "%s %lf\n", ctmp, &dtmp);
              if (CheckWords(ctmp, "NVMCCalMode") == 0) {
                bufInt[IdxVMCCalcMode] = (int) dtmp;
              } else if (CheckWords(ctmp, "NLanczosMode") == 0) {
                bufInt[IdxLanczosMode] = (int) dtmp;
              } else if (CheckWords(ctmp, "NDataIdxStart") == 0) {
                bufInt[IdxDataIdxStart] = (int) dtmp;
              } else if (CheckWords(ctmp, "NDataQtySmp") == 0) {
                bufInt[IdxDataQtySmp] = (int) dtmp;
              } else if (CheckWords(ctmp, "Nsite") == 0) {
                bufInt[IdxNsite] = (int) dtmp;
              } else if (CheckWords(ctmp, "Ne") == 0 || CheckWords(ctmp, "Nelectron") == 0) {
                bufInt[IdxNe] = (int) dtmp;
              } else if (CheckWords(ctmp, "NSPGaussLeg") == 0) {
                bufInt[IdxSPGaussLeg] = (int) dtmp;
              } else if (CheckWords(ctmp, "NSPStot") == 0) {
                bufInt[IdxSPStot] = (int) dtmp;
              } else if (CheckWords(ctmp, "NMPTrans") == 0) {
                bufInt[IdxMPTrans] = (int) dtmp;
              } else if (CheckWords(ctmp, "NSROptItrStep") == 0) {
                bufInt[IdxSROptItrStep] = (int) dtmp;
              } else if (CheckWords(ctmp, "NSROptItrSmp") == 0) {
                bufInt[IdxSROptItrSmp] = (int) dtmp;
              } else if (CheckWords(ctmp, "DSROptRedCut") == 0) {
                bufDouble[IdxSROptRedCut] = (double) dtmp;
              } else if (CheckWords(ctmp, "DSROptStaDel") == 0) {
                bufDouble[IdxSROptStaDel] = (double) dtmp;
              } else if (CheckWords(ctmp, "DSROptStepDt") == 0) {
                bufDouble[IdxSROptStepDt] = (double) dtmp;
              } else if (CheckWords(ctmp, "NVMCWarmUp") == 0) {
                bufInt[IdxVMCWarmUp] = (int) dtmp;
              } else if (CheckWords(ctmp, "NVMCInterval") == 0) {
                bufInt[IdxVMCInterval] = (int) dtmp;
              } else if (CheckWords(ctmp, "NVMCSample") == 0) {
                bufInt[IdxVMCSample] = (int) dtmp;
              } else if (CheckWords(ctmp, "NExUpdatePath") == 0) {
                bufInt[IdxExUpdatePath] = (int) dtmp;
              } else if (CheckWords(ctmp, "RndSeed") == 0) {
                bufInt[IdxRndSeed] = (int) dtmp;
              } else if (CheckWords(ctmp, "NSplitSize") == 0) {
                bufInt[IdxSplitSize] = (int) dtmp;
              } else if (CheckWords(ctmp, "NStore") == 0) {
                NStoreO = (int) dtmp;
              } else {
                fprintf(stderr, "  Error: keyword \" %s \" is incorrect. \n", ctmp);
                info = ReadDefFileError(defname);
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
              }
            }
            if (bufInt[IdxRndSeed] < 0) {
              bufInt[IdxRndSeed] = (int) time(NULL);
              fprintf(stdout, "  remark: Seed = %d\n", bufInt[IdxRndSeed]);
            }
            fclose(fp);
            break;//modpara file

          case KWLocSpin:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNLocSpin]);
            fclose(fp);
            break;

          case KWTrans:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNTrans]);
            fclose(fp);
            break;

          case KWCoulombIntra:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNCoulombIntra]);
            fclose(fp);
            break;

          case KWCoulombInter:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNCoulombInter]);
            fclose(fp);
            break;

          case KWHund:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNHund]);
            fclose(fp);
            break;

          case KWPairHop:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNPairHop]);
            fclose(fp);
            break;

          case KWExchange:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNExchange]);
            fclose(fp);
            break;

          case KWGutzwiller:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNGutz]);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &iComplexFlgGutzwiller);
            fclose(fp);
            break;

          case KWJastrow:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNJast]);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &iComplexFlgJastrow);
            fclose(fp);
            break;

          case KWDH2:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNDH2]);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &iComplexFlgDH2);
            fclose(fp);
            break;

          case KWDH4:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNDH4]);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &iComplexFlgDH4);
            fclose(fp);
            break;

            /*
          case KWOrbital:
            fgets(ctmp, sizeof(ctmp)/sizeof(char), fp);
            fgets(ctmp2, sizeof(ctmp2)/sizeof(char), fp);
            sscanf(ctmp2,"%s %d\n", ctmp, &bufInt[IdxNOrbit]);
            fgets(ctmp2, sizeof(ctmp2)/sizeof(char), fp);
            sscanf(ctmp2,"%s %d\n", ctmp, &iComplexFlgOrbital);
            fclose(fp);
            break;
            */

          case KWOrbital:
          case KWOrbitalAntiParallel:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNOrbitAntiParallel]);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &iOrbitalComplex);
            fclose(fp);
            iFlgOrbitalAP = 1;
            iNOrbitalAP = bufInt[IdxNOrbitAntiParallel];
            bufInt[IdxNOrbit] += bufInt[IdxNOrbitAntiParallel];
            iComplexFlgOrbital += iOrbitalComplex;
            if (iFlgOrbitalSimple == -1) {
              fprintf(stderr, "error: Multiple definition of Orbital files.\n");
              info = ReadDefFileError(defname);
            }
            break;

          case KWOrbitalGeneral:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNOrbit]);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &iOrbitalComplex);
            fclose(fp);
            iFlgOrbitalGeneral = 1;
            iComplexFlgOrbital = iOrbitalComplex;
            if (iFlgOrbitalSimple == 1) {
              fprintf(stderr, "error: Multiple definition of Orbital files.\n");
              info = ReadDefFileError(defname);
            }
            iFlgOrbitalSimple = -1;
            break;

          case KWOrbitalParallel:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNOrbitParallel]);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &iOrbitalComplex);
            fclose(fp);
            //up-up and down-down
            iNOrbitalP = bufInt[IdxNOrbitParallel];
            bufInt[IdxNOrbit] += 2 * bufInt[IdxNOrbitParallel];
            if (bufInt[IdxNOrbitParallel] > 0) {
              iFlgOrbitalGeneral = 1;
            }
            iComplexFlgOrbital += iOrbitalComplex;
            iFlgOrbitalAP = 1;

            if (iFlgOrbitalSimple == -1) {
              fprintf(stderr, "error: Multiple definition of Orbital files.\n");
              info = ReadDefFileError(defname);
            }
            break;

          case KWTransSym:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNQPTrans]);
            fclose(fp);
            break;

          case KWOneBodyG:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNOneBodyG]);
            fclose(fp);
            break;

          case KWTwoBodyG:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNTwoBodyG]);
            fclose(fp);
            break;


          case KWTwoBodyGEx:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNTwoBodyGEx]);
            fclose(fp);
            break;

          case KWInterAll:
            cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
            sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNInterAll]);
            fclose(fp);
            break;

          case KWOptTrans:
            bufInt[IdxNQPOptTrans] = 1;
            if (FlagOptTrans > 0) {
              cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
              cerr = fgets(ctmp2, sizeof(ctmp2) / sizeof(char), fp);
              sscanf(ctmp2, "%s %d\n", ctmp, &bufInt[IdxNQPOptTrans]);
              if (bufInt[IdxNQPOptTrans] < 1) {
                fprintf(stderr, "error: NQPOptTrans should be larger than 0.\n");
                info = ReadDefFileError(defname);
              }
            }
            fclose(fp);
            break;

          case KWBFRange:
#ifdef _NOTBACKFLOW
            fprintf(stderr, "error: Back Flow is not supported.\n");
            info = ReadDefFileError(defname);
#else
          fgets(ctmp, sizeof(ctmp)/sizeof(char), fp);
          fgets(ctmp2, sizeof(ctmp2)/sizeof(char), fp);
          sscanf(ctmp2,"%s %d %d\n", ctmp, &bufInt[IdxNrange], &bufInt[IdxNNz]);
#endif
            fclose(fp);
            break;

          case KWBF:
#ifdef _NOTBACKFLOW
            fprintf(stderr, "error: Back Flow is not supported.\n");
            info = ReadDefFileError(defname);
#else
          fgets(ctmp, sizeof(ctmp)/sizeof(char), fp);
          fgets(ctmp2, sizeof(ctmp2)/sizeof(char), fp);
          sscanf(ctmp2,"%s %d\n", ctmp, &bufInt[IdxNBF]);
#endif
            fclose(fp);
            break;


          default:
            fclose(fp);
            break;
        }//case KW
      }
    }

    //For Lanczos mode: Calculation of Green's function

    if (bufInt[IdxLanczosMode] > 1) {
      //Get info of CisAjs and CisAjsCktAltDC
      int i;
      NCisAjsLz = bufInt[IdxNOneBodyG];
      //bufInt[IdxNTwoBodyGEx] = bufInt[IdxNTwoBodyG];
      CisAjsLzIdx = malloc(sizeof(int *) * NCisAjsLz);
      for (i = 0; i < NCisAjsLz; i++) {
        CisAjsLzIdx[i] = malloc(sizeof(int) * 4);
      }
      iOneBodyGIdx = malloc(sizeof(int *) * (2 * bufInt[IdxNsite])); //For spin
      //pInt=iFlgOneBodyG[0];
      for (i = 0; i < 2 * bufInt[IdxNsite]; i++) {
        iOneBodyGIdx[i] = malloc(sizeof(int) * (2 * bufInt[IdxNsite]));
      }
      bufInt[IdxNOneBodyG] = CountOneBodyGForLanczos(xNameListFile, NCisAjsLz, bufInt[IdxNTwoBodyG],
                                                     bufInt[IdxNsite], CisAjsLzIdx, iOneBodyGIdx);
    }

  }

  if (info != 0) {
    if (rank == 0) {
      fprintf(stderr, "error: Definition files(*.def) are incomplete.\n");
    }
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  if (rank == 0) {
    AllComplexFlag = iComplexFlgGutzwiller + iComplexFlgJastrow + iComplexFlgDH2; //TBC
    AllComplexFlag += iComplexFlgDH4 + iComplexFlgOrbital;//TBC
    //AllComplexFlag  = 1;//DEBUG
    // AllComplexFlag= 0 -> All real, !=0 -> complex
  }

#ifdef _mpi_use
  MPI_Bcast(bufInt, nBufInt, MPI_INT, 0, comm);
  MPI_Bcast(&NStoreO, 1, MPI_INT, 0, comm); // for NStoreO
  MPI_Bcast(&AllComplexFlag, 1, MPI_INT, 0, comm); // for Real
  MPI_Bcast(&iFlgOrbitalGeneral, 1, MPI_INT, 0, comm); // for fsz
  MPI_Bcast(bufDouble, nBufDouble, MPI_DOUBLE, 0, comm);
  MPI_Bcast(CDataFileHead, nBufChar, MPI_CHAR, 0, comm);
  MPI_Bcast(CParaFileHead, nBufChar, MPI_CHAR, 0, comm);
#endif /* _mpi_use */

  NVMCCalMode = bufInt[IdxVMCCalcMode];
  NLanczosMode = bufInt[IdxLanczosMode];
  NDataIdxStart = bufInt[IdxDataIdxStart];
  NDataQtySmp = bufInt[IdxDataQtySmp];
  Nsite = bufInt[IdxNsite];
  Ne = bufInt[IdxNe];
  NSPGaussLeg = bufInt[IdxSPGaussLeg];
  NSPStot = bufInt[IdxSPStot];
  NMPTrans = bufInt[IdxMPTrans];
  NSROptItrStep = bufInt[IdxSROptItrStep];
  NSROptItrSmp = bufInt[IdxSROptItrSmp];
  NSROptFixSmp = bufInt[IdxSROptFixSmp];
  NVMCWarmUp = bufInt[IdxVMCWarmUp];
  NVMCInterval = bufInt[IdxVMCInterval];
  NVMCSample = bufInt[IdxVMCSample];
  NExUpdatePath = bufInt[IdxExUpdatePath];
  RndSeed = bufInt[IdxRndSeed];
  NSplitSize = bufInt[IdxSplitSize];
  NLocSpn = bufInt[IdxNLocSpin];
  NTransfer = bufInt[IdxNTrans];
  NCoulombIntra = bufInt[IdxNCoulombIntra];
  NCoulombInter = bufInt[IdxNCoulombInter];
  NHundCoupling = bufInt[IdxNHund];
  //NPairHopping = bufInt[IdxNPairHop];
  NPairHopping = 2*bufInt[IdxNPairHop];
  NExchangeCoupling = bufInt[IdxNExchange];
  NGutzwillerIdx = bufInt[IdxNGutz];
  NJastrowIdx = bufInt[IdxNJast];
  NDoublonHolon2siteIdx = bufInt[IdxNDH2];
  NDoublonHolon4siteIdx = bufInt[IdxNDH4];
  NOrbitalIdx = bufInt[IdxNOrbit];
  NQPTrans = bufInt[IdxNQPTrans];
  NCisAjs = bufInt[IdxNOneBodyG];
  //fprintf(stdout, "Debug: NCisAjs=%d\n", NCisAjs);
  NCisAjsCktAlt = bufInt[IdxNTwoBodyGEx];
  NCisAjsCktAltDC = bufInt[IdxNTwoBodyG];
  NInterAll = bufInt[IdxNInterAll];
  NQPOptTrans = bufInt[IdxNQPOptTrans];
  Nrange = bufInt[IdxNrange];
  NBackFlowIdx = bufInt[IdxNBF];
  Nz = bufInt[IdxNNz];
  DSROptRedCut = bufDouble[IdxSROptRedCut];
  DSROptStaDel = bufDouble[IdxSROptStaDel];
  DSROptStepDt = bufDouble[IdxSROptStepDt];

  if (NMPTrans < 0) {
    APFlag = 1; /* anti-periodic boundary */
    NMPTrans *= -1;
  } else {
    APFlag = 0;
  }

  if (DSROptStepDt < 0) {
    SRFlag = 1; /* diagonalization */
    if (rank == 0) fprintf(stderr, "remark: Diagonalization Mode\n");
    DSROptStepDt *= -1;
  } else {
    SRFlag = 0;
  }
  Nsize = 2 * Ne;
  Nsite2 = 2 * Nsite;
  NSlater = NOrbitalIdx;
  NProj = NGutzwillerIdx + NJastrowIdx
          + 2 * 3 * NDoublonHolon2siteIdx
          + 2 * 5 * NDoublonHolon4siteIdx;
  NOptTrans = (FlagOptTrans > 0) ? NQPOptTrans : 0;

  /* [s] For BackFlow */
  if (NBackFlowIdx > 0) {
    NrangeIdx = 3 * (Nrange - 1) / Nz + 1; //For Nz-conectivity
    NBFIdxTotal = (NrangeIdx - 1) * (NrangeIdx) / 2 + (NrangeIdx);
    NProjBF = NBFIdxTotal * NBackFlowIdx;
  } else {
    NrangeIdx = 0;
    NBFIdxTotal = 0;
    NProjBF = 0;
  }
  /* [e] For BackFlow */

  NPara = NProj + NSlater + NOptTrans + NProjBF;
  NQPFix = NSPGaussLeg * NMPTrans;
  NQPFull = NQPFix * NQPOptTrans;
  SROptSize = NPara + 1;

  NTotalDefInt = Nsite /* LocSpn */
    + 4*NTransfer /* Transfer */
    + NCoulombIntra /* CoulombIntra */
    + 2*NCoulombInter /* CoulombInter */
    + 2*NHundCoupling /* HundCoupling */
    + 2*NPairHopping /* PairHopping */
    + 2*NExchangeCoupling /* ExchangeCoupling */
    + Nsite /* GutzwillerIdx */
    + Nsite*Nsite /* JastrowIdx */
    + 2*Nsite*NDoublonHolon2siteIdx /* DoublonHolon2siteIdx */
    + 4*Nsite*NDoublonHolon4siteIdx /* DoublonHolon4siteIdx */
    //+ (2*Nsite)*(2*Nsite) /* OrbitalIdx */ //fsz
    //+ (2*Nsite)*(2*Nsite) /* OrbitalSgn */ //fsz
    + Nsite*NQPTrans /* QPTrans */
    + Nsite * NQPTrans /* QPTransInv */
    + Nsite*NQPTrans /* QPTransSgn */
    + 4*NCisAjs /* CisAjs */
    + 8*NCisAjsCktAlt /* CisAjsCktAlt */
    + 8*NCisAjsCktAltDC /* CisAjsCktAltDC */
    + 8*NInterAll /* InterAll */
    + Nsite*NQPOptTrans /* QPOptTrans */
    + Nsite*NQPOptTrans /* QPOptTransSgn */
    + 2*NPara; /* OptFlag */ // TBC

  //Orbitalidx
  if(iFlgOrbitalGeneral==0){
    NTotalDefInt += Nsite*Nsite; // OrbitalIdx
    NTotalDefInt += Nsite*Nsite; // OrbitalSgn
  }
  else if(iFlgOrbitalGeneral==1){
    NTotalDefInt += (2*Nsite)*(2*Nsite); //OrbitalIdx
    NTotalDefInt += (2*Nsite)*(2*Nsite); //OrbitalSgn
  }

  if (NBackFlowIdx > 0) {
    NTotalDefInt +=
            Nsite * Nsite * Nsite * Nsite; /* BackflowIdx */
  }

  NTotalDefDouble =
          NCoulombIntra /* ParaCoulombIntra */
          + NCoulombInter /* ParaCoulombInter */
          + NHundCoupling /* ParaHondCoupling */
          + NPairHopping  /* ParaPairHopping */
          + NExchangeCoupling /* ParaExchangeCoupling */
          //    + NQPTrans /* ParaQPTrans */
          //+ NInterAll /* ParaInterAll */
          + NQPOptTrans; /* ParaQPTransOpt */

  return 0;
}

int ReadDefFileIdxPara(char *xNameListFile, MPI_Comm comm){
  FILE *fp;
  char defname[D_FileNameMax];
  char ctmp[D_FileNameMax], ctmp2[256];
  int itmp;
  int iKWidx=0;
  char *cerr;
  int ierr;
  
  int i,j,n,idx,idx0,idx1,info=0;
  int isite1, isite2, idxLanczos;
  int fidx=0; /* index for OptFlag */
  int count_idx=0;
  int x0,x1,x2,x3,x4,x5,x6,x7;
  double dReValue, dImValue;
  int rank;
  int sgn;

	//[s] for Orbital idx
	int all_i, all_j;
	int iSzFreeFlg=0; // 0: SzConserved, 1:SzFree
	int fij=0;
	int fijSign=1;
    int spn_i,spn_j;
	//[e] for Orbital idx

  MPI_Comm_rank(comm, &rank);
  
  if(rank==0) {
    for (iKWidx = KWLocSpin; iKWidx < KWIdxInt_end; iKWidx++) {
      strcpy(defname, cFileNameListFile[iKWidx]);

      if (strcmp(defname, "") == 0) continue;

      fp = fopen(defname, "r");
      if (fp == NULL) {
        info = ReadDefFileError(defname);
        fclose(fp);
        continue;
      }

      /*=======================================================================*/
      for(i=0;i<IgnoreLinesInDef;i++) fgets(ctmp, sizeof(ctmp)/sizeof(char), fp);
      idx=0;
      idx0=0;
      switch(iKWidx){
      case KWLocSpin:
	/* Read locspn.def----------------------------------------*/
	while( fgets(ctmp2, sizeof(ctmp2)/sizeof(char), fp) != NULL){
	  sscanf(ctmp2, "%d %d\n", &(x0), &(x1) );
      LocSpn[x0] = x1;
      if(CheckSite(x0, Nsite)!=0){
          fprintf(stderr, "Error: Site index is incorrect.\n");
          info=1;
		  break;
      }
	  idx++;
	}
	if(NLocSpn>2*Ne){
	  fprintf(stderr, "Error: 2*Ne must satisfy the condition, 2*Ne >= NLocalSpin.\n");
	  info=1;
	}
	if(NLocSpn>0 && NExUpdatePath==0){
	  fprintf(stderr, "Error: NExUpdatePath (in modpara.def) must be 1.\n");
	  info=1;
	}
	if(idx!=Nsite) info = ReadDefFileError(defname);
	fclose(fp);	  
	break;//locspn
	      
      case KWTrans:
	/* transfer.def--------------------------------------*/
	if(NTransfer>0){
	  while( fscanf(fp, "%d %d %d %d %lf %lf\n",
			&x0,&x1,&x2,&x3,
			&dReValue, &dImValue)!=EOF){
		  Transfer[idx][0]=x0;
		  Transfer[idx][1]=x1;
		  Transfer[idx][2]=x2;
		  Transfer[idx][3]=x3;
		  ParaTransfer[idx]=dReValue+I*dImValue;

      if(CheckPairSite(x0, x2, Nsite) !=0) {
			  fprintf(stderr, "Error: Site index is incorrect. \n");
			  info = 1;
			  break;
		  }
	    idx++;
	  }
	  if(idx!=NTransfer) info = ReadDefFileError(defname);
	}
	fclose(fp);
	break;

      case KWCoulombIntra:
	/*coulombintra.def----------------------------------*/
	if(NCoulombIntra>0){
	  while( fscanf(fp, "%d %lf\n", 
			&x0, &dReValue )!=EOF){
		  CoulombIntra[idx]=x0;
          if(CheckSite(x0, Nsite) != 0){
              fprintf(stderr, "Error: Site index is incorrect. \n");
              info=1;
			  break;
		  }
		  ParaCoulombIntra[idx]=dReValue;
	    idx++;
	  }
        if(idx!=NCoulombIntra) info = ReadDefFileError(defname);
	}
	fclose(fp);
	break;

      case KWCoulombInter:
	/*coulombinter.def----------------------------------*/
	if(NCoulombInter>0){
	  while(fgets(ctmp2, sizeof(ctmp2)/sizeof(char), fp) != NULL){
	    sscanf(ctmp2, "%d %d %lf\n",
		   &x0,&x1, &dReValue);
		  CoulombInter[idx][0]=x0;
		  CoulombInter[idx][1]=x1;
          if(CheckPairSite(x0, x1, Nsite) !=0) {
              fprintf(stderr, "Error: Site index is incorrect. \n");
              info=1;
			  break;
		  }
          ParaCoulombInter[idx]=dReValue;
	    idx++;
	  }
	  if(idx!=NCoulombInter) info=ReadDefFileError(defname);
	}
	fclose(fp);
	break;

      case KWHund:
	/*hund.def------------------------------------------*/
	if(NHundCoupling>0){
	  while( fscanf(fp, "%d %d %lf\n",
			&x0,&x1, &dReValue)!=EOF){
			HundCoupling[idx][0]=x0;
			HundCoupling[idx][1]=x1;
          if(CheckPairSite(x0, x1, Nsite) !=0) {
              fprintf(stderr, "Error: Site index is incorrect. \n");
              info=1;
			  break;
		  }
          ParaHundCoupling[idx]=dReValue;
	    idx++;
	  }
	  if(idx!=NHundCoupling) info=ReadDefFileError(defname);
	}
	fclose(fp);
	break;

      case KWPairHop:
        /*pairhop.def---------------------------------------*/
        if (NPairHopping > 0) {
          while (fscanf(fp, "%d %d %lf\n",
                        &x0, &x1, &dReValue) != EOF) {
            PairHopping[2*idx][0] = x0;
            PairHopping[2*idx][1] = x1;
            if (CheckPairSite(x0, x1, Nsite) != 0) {
              fprintf(stderr, "Error: Site index is incorrect. \n");
              info = 1;
              break;
            }
            ParaPairHopping[2*idx] = dReValue;
            PairHopping[2*idx+1][0] = x1;
            PairHopping[2*idx+1][1] = x0;
            ParaPairHopping[2*idx+1] = dReValue;
            idx++;
          }
          if (2*idx != NPairHopping) info = ReadDefFileError(defname);
        }
        fclose(fp);
        break;
	
      case KWExchange:
	/*exchange.def--------------------------------------*/
	if(NExchangeCoupling>0){
	  while( fscanf(fp, "%d %d %lf\n",
					&x0,&x1, &dReValue)!=EOF){
		  	ExchangeCoupling[idx][0]=x0;
			ExchangeCoupling[idx][1]=x1;
		  if(CheckPairSite(x0, x1, Nsite) !=0) {
			  fprintf(stderr, "Error: Site index is incorrect. \n");
			  info=1;
			  break;
		  }
		  ParaExchangeCoupling[idx]=dReValue;
	    idx++;
	  }
	  if(idx!=NExchangeCoupling) info=ReadDefFileError(defname);
	}
	fclose(fp);
	break;

      case KWGutzwiller:
	/*gutzwilleridx.def---------------------------------*/
	if(NGutzwillerIdx>0){
	  idx0=idx1=0;
	  while( fscanf(fp, "%d ", &i) != EOF){
	    fscanf(fp, "%d\n", &(GutzwillerIdx[i]));
		  if(CheckSite(i, Nsite) !=0){
			  fprintf(stderr, "Error: Site index is incorrect. \n");
			  info=1;
			  break;
		  }
	    idx0++;
	    if(idx0==Nsite) break;
	  }
      fidx=0;
	  while( fscanf(fp, "%d ", &i) != EOF){
	    fscanf(fp, "%d\n", &(OptFlag[2*fidx])); // TBC real

	    OptFlag[2*fidx+1] = iComplexFlgGutzwiller; //  TBC imaginary
	    fidx++;
	    idx1++;
        count_idx++;
	  }
	  if(idx0!=Nsite || idx1!=NGutzwillerIdx) {
	    info=ReadDefFileError(defname);
	  }
	}
	fclose(fp);
	break;

      case KWJastrow:
	/*jastrowidx.def------------------------------------*/
	if(NJastrowIdx>0){
	  idx0 = idx1 = 0;
	  while( fscanf(fp, "%d %d ", &i, &j) != EOF){
	    if(i==j){
	      fprintf(stderr, "Error in %s: [Condition] i neq j\n", defname);
	      info=1;
	      break;
	    }
		  if(CheckPairSite(i, j, Nsite) !=0) {
			  fprintf(stderr, "Error: Site index is incorrect. \n");
			  info=1;
			  break;
		  }

		  fscanf(fp, "%d\n", &(JastrowIdx[i][j]));
	    JastrowIdx[i][i] = -1; // This case is Gutzwiller.
	    idx0++;
	    if(idx0==Nsite*(Nsite-1)) break;
	  }
      fidx=NGutzwillerIdx;
	  while( fscanf(fp, "%d ", &i) != EOF){
	    fscanf(fp, "%d\n", &(OptFlag[2*fidx])); // TBC real
	    OptFlag[2*fidx+1] = iComplexFlgJastrow; //  TBC imaginary
	    //	    OptFlag[2*fidx+1] = 0; //  TBC imaginary
	    fidx++;
	    idx1++;
          count_idx++;

      }
	  if(idx0!=Nsite*(Nsite-1) || idx1!=NJastrowIdx) {
	    info=ReadDefFileError(defname);
	  }
	}
	fclose(fp);
	break;
	  
      case KWDH2:
	/*doublonholon2siteidx.def--------------------------*/
	if(NDoublonHolon2siteIdx>0) {
		idx0 = idx1 = 0;
		while (fscanf(fp, "%d %d %d %d\n", &i, &(x0), &(x1), &n) != EOF) {
			DoublonHolon2siteIdx[n][2 * i] = x0;
			DoublonHolon2siteIdx[n][2 * i + 1] = x1;
			if (CheckSite(i, Nsite) != 0 || CheckPairSite(x0, x1, Nsite) != 0) {
				fprintf(stderr, "Error: Site index is incorrect. \n");
				info = 1;
				break;
			}
			idx0++;
			if (idx0 == Nsite * NDoublonHolon2siteIdx) break;
		}
		fidx = NGutzwillerIdx + NJastrowIdx;
		while (fscanf(fp, "%d ", &i) != EOF) {
			fscanf(fp, "%d\n", &(OptFlag[2 * fidx]));//TBC real
			OptFlag[2 * fidx + 1] = iComplexFlgDH2; //  TBC imaginary
			//OptFlag[2*fidx+1] = 0; //  TBC imaginary
			fidx++;
			idx1++;
			count_idx++;
		}
		if (idx0 != Nsite * NDoublonHolon2siteIdx
			|| idx1 != 2 * 3 * NDoublonHolon2siteIdx) {
			info = ReadDefFileError(defname);
		}
	}
	fclose(fp);
	break;

      case KWDH4:
	/*doublonholon4siteidx.def--------------------------*/
	if(NDoublonHolon4siteIdx>0){
	  idx0 = idx1 = 0;
	  while( fscanf(fp, "%d %d %d %d %d %d\n",
			&i, &(x0), &(x1), &(x2), &(x3), &n) != EOF) {
		  DoublonHolon4siteIdx[n][4 * i] = x0;
		  DoublonHolon4siteIdx[n][4 * i + 1] = x1;
		  DoublonHolon4siteIdx[n][4 * i + 2] = x2;
		  DoublonHolon4siteIdx[n][4 * i + 3] = x3;
		  if (CheckSite(i, Nsite) != 0 || CheckQuadSite(x0, x1, x2, x3, Nsite) != 0) {
			  fprintf(stderr, "Error: Site index is incorrect. \n");
			  info = 1;
			  break;
		  }
		  idx0++;
		  if (idx0 == Nsite * NDoublonHolon4siteIdx) break;
	  }
      fidx=NGutzwillerIdx+NJastrowIdx+2*3*NDoublonHolon2siteIdx;
        while( fscanf(fp, "%d ", &i) != EOF){
	    fscanf(fp, "%d\n", &(OptFlag[2*fidx]));
	    OptFlag[2*fidx+1] = iComplexFlgDH4; //  TBC imaginary
	    //OptFlag[2*fidx+1] = 0; //  TBC imaginary
	    fidx++;
	    idx1++;
            count_idx++;
        }
	  if(idx0!=Nsite*NDoublonHolon4siteIdx
	     || idx1!=2*5*NDoublonHolon4siteIdx) {
	    info=ReadDefFileError(defname);
	  }
	}
	fclose(fp);
	break;


  case KWOrbital:        
  case KWOrbitalAntiParallel:
  /*orbitalidxs.def------------------------------------*/
  if(iNOrbitalAP>0){
    idx0 = idx1 = 0;
    itmp=0;
    if(iFlgOrbitalGeneral==0){
      if(APFlag==0) {
         while( fscanf(fp, "%d %d %d\n", &i, &j, &fij) != EOF){
		  		 spn_i = 0;
			  	 spn_j = 1;

			     all_i = i+spn_i*Nsite; //fsz
           all_j = j+spn_j*Nsite; //fsz
           if(CheckPairSite(i, j, Nsite) != 0){
             fprintf(stderr, "Error: Site index is incorrect. \n");
             info=1;
             break;
           }
           if(all_i >= all_j){
             itmp=1;
           }
           idx0++;
           OrbitalIdx[i][j] = fij;
           OrbitalSgn[i][j] = 1;
           if(idx0==Nsite*Nsite) break;
         }
      }else { /* anti-periodic boundary mode */
         while( fscanf(fp, "%d %d %d %d\n", &i, &j, &fij, &fijSign) != EOF){
				   spn_i = 0;
				   spn_j = 1;
				   all_i = i+spn_i*Nsite; //fsz
           all_j = j+spn_j*Nsite; //fsz
           if(all_i >= all_j){
             itmp=1;
           }
           idx0++;
           OrbitalIdx[i][j]=fij;
           OrbitalSgn[i][j] = fijSign;
           if(idx0==Nsite*Nsite) break;
         }
      }
      fidx=NProj;
      while( fscanf(fp, "%d ", &i) != EOF){
         fscanf(fp, "%d\n", &(OptFlag[2*fidx]));
         OptFlag[2*fidx+1] = iComplexFlgOrbital; //  TBC imaginary
         //OptFlag[2*fidx+1] = 0; //  TBC imaginary
         fidx ++;
         idx1++;
         count_idx++;
      }
    }else{// general orbital
      if(APFlag==0) {
        while( fscanf(fp, "%d %d %d\n", &i, &j, &fij) != EOF){
          spn_i = 0;
          spn_j = 1;
          all_i = i+spn_i*Nsite; //fsz
          all_j = j+spn_j*Nsite; //fsz
          if(CheckPairSite(i, j, Nsite) != 0){
            fprintf(stderr, "Error: Site index is incorrect. \n");
            info=1;
            break;
          }
          if(all_i >= all_j){
            itmp=1;
          }
          idx0++;
          OrbitalIdx[all_i][all_j]=fij;
          OrbitalSgn[all_i][all_j] = 1;
          // Note F_{IJ}=-F_{JI}
          OrbitalIdx[all_j][all_i]=fij;
          OrbitalSgn[all_j][all_i] = -1;
          if(idx0==(Nsite*Nsite)) break; 
        }
      }else { /* anti-periodic boundary mode */
        while( fscanf(fp, "%d %d %d %d \n", &i, &j, &fij, &fijSign) != EOF){
          spn_i = 0;
          spn_j = 1;
          all_i = i+spn_i*Nsite; //fsz
          all_j = j+spn_j*Nsite; //fsz
          if(all_i >= all_j){
            itmp=1;
          }
          idx0++;
          OrbitalIdx[all_i][all_j]=fij;
          OrbitalSgn[all_i][all_j] = fijSign;
          // Note F_{IJ}=-F_{JI}
          OrbitalIdx[all_j][all_i]=fij;
          OrbitalSgn[all_j][all_i] = -fijSign;
          if(idx0==(Nsite*(Nsite))) break; //2N*(2N-1)/2
       }
     }
     fidx=NProj;
     while( fscanf(fp, "%d ", &i) != EOF){
       fscanf(fp, "%d\n", &(OptFlag[2*fidx]));
       OptFlag[2*fidx+1] = iComplexFlgOrbital; //  TBC imaginary
       //OptFlag[2*fidx+1] = 0; //  TBC imaginary
       fidx ++;
       idx1++;
       count_idx++;
     }
   }
   if(idx0!=(Nsite*Nsite) || idx1!=iNOrbitalAP || itmp==1) {
     info=ReadDefFileError(defname);
   }
  }
  fclose(fp);
  break;

      case KWOrbitalGeneral:
     idx0 = idx1 = 0;
     itmp=0;
     //printf("idx0=%d idx1=%d itmp=%d \n",idx0,idx1,itmp);
          if(APFlag==0) {
            while( fscanf(fp, "%d %d %d %d %d\n", &i, &spn_i, &j, &spn_j, &fij) != EOF){
                all_i = i+spn_i*Nsite; //fsz
                all_j = j+spn_j*Nsite; //fsz
                if(CheckPairSite(i, j, Nsite) != 0){
                  fprintf(stderr, "Error: Site index is incorrect. \n");
                  printf("XDEBUG: %d %d %d %d %d\n",i,j,all_i,all_j,Nsite);
                  break;
              }
              if (all_i >= all_j) {
                itmp = 1;
              }
              idx0++;
              OrbitalIdx[all_i][all_j] = fij;
              OrbitalSgn[all_i][all_j] = 1;
              // Note F_{IJ}=-F_{JI}
              OrbitalIdx[all_j][all_i] = fij;
              OrbitalSgn[all_j][all_i] = -1;
              if (idx0 == (2 * Nsite * Nsite - Nsite)) break;
            }
          } else { /* anti-periodic boundary mode */
            while (fscanf(fp, "%d %d %d %d \n", &i, &j, &fij, &fijSign) != EOF) {
              spn_i = 0;
              spn_j = 1;
              all_i = i + spn_i * Nsite; //fsz
              all_j = j + spn_j * Nsite; //fsz
              if (all_i >= all_j) {
                itmp = 1;
              }
              idx0++;
              OrbitalIdx[all_i][all_j] = fij;
              OrbitalSgn[all_i][all_j] = fijSign;
              // Note F_{IJ}=-F_{JI}
              OrbitalIdx[all_j][all_i] = fij;
              OrbitalSgn[all_j][all_i] = -fijSign;
              if (idx0 == (2 * Nsite * Nsite - Nsite)) break; //2N*(2N-1)/2
            }
          }
          fidx = NProj;
          while (fscanf(fp, "%d ", &i) != EOF) {
            ierr = fscanf(fp, "%d\n", &(OptFlag[2 * fidx]));
            OptFlag[2 * fidx + 1] = iComplexFlgOrbital; //  TBC imaginary
            //OptFlag[2*fidx+1] = 0; //  TBC imaginary
            fidx++;
            idx1++;
            count_idx++;
          }

          if (idx0 != (2 * Nsite * Nsite - Nsite) || idx1 != NOrbitalIdx || itmp == 1) {
            info = ReadDefFileError(defname);
          }
          fclose(fp);

          break;

        case KWOrbitalParallel:

          /*orbitalidxt.def------------------------------------*/
          if (iNOrbitalP > 0) {
            idx0 = idx1 = 0;

            while (fscanf(fp, "%d %d %d\n", &i, &j, &fij) != EOF) {
              for (spn_i = 0; spn_i < 2; spn_i++) {
                all_i = i + spn_i * Nsite; //fsz
                all_j = j + spn_i * Nsite; //fsz
                if (CheckPairSite(i, j, Nsite) != 0) {
                  fprintf(stderr, "Error: Site index is incorrect. \n");
                  info = 1;
//>>>>>>> develop
                  break;
                }
                if (all_i >= all_j) {
                  itmp = 1;
                }
                idx0++;
                fij = iNOrbitalAP+2*fij+spn_i;
                OrbitalIdx[all_i][all_j] = fij;
                OrbitalSgn[all_i][all_j] = 1;
                // Note F_{IJ}=-F_{JI}
                OrbitalIdx[all_j][all_i] = fij;
                OrbitalSgn[all_j][all_i] = -1;
                if (idx0 == (Nsite * (Nsite - 1)) / 2) break;
              }
            }
          } else { /* anti-periodic boundary mode */
            while (fscanf(fp, "%d %d %d %d \n", &i, &j, &fij, &fijSign) != EOF) {
              for (spn_i = 0; spn_i < 2; spn_i++) {
                all_i = i + spn_i * Nsite; //fsz
                all_j = j + spn_i * Nsite; //fsz
                if (all_i >= all_j) {
                  itmp = 1;
                }
                idx0++;
                fij = iNOrbitalAP+2*fij+spn_i;
                OrbitalIdx[all_i][all_j] = fij;
                OrbitalSgn[all_i][all_j] = fijSign;
                // Note F_{IJ}=-F_{JI}
                OrbitalIdx[all_j][all_i] = fij;
                OrbitalSgn[all_j][all_i] = -fijSign;
                if (idx0 == ((Nsite * (Nsite - 1)) / 2)) break; //N*(N-1)
              }
            }
          }

          fidx = NProj+iNOrbitalAP;
          while (fscanf(fp, "%d ", &i) != EOF) {
            ierr = fscanf(fp, "%d\n", &(OptFlag[2 * fidx]));
            OptFlag[2 * fidx + 1] = iComplexFlgOrbital; //  TBC imaginary
            //OptFlag[2*fidx+1] = 0; //  TBC imaginary
            fidx++;
            idx1++;
            count_idx++;
          }

          if (idx0 != (Nsite * (Nsite - 1)) / 2 || idx1 != iNOrbitalP) {
            info = ReadDefFileError(defname);
          }
          fclose(fp);
          break;


        case KWTransSym:
          /*qptransidx.def------------------------------------*/
          if (NQPTrans > 0) {
            for (i = 0; i < NQPTrans; i++) {
              itmp = 0;
              dReValue = 0;
              dImValue = 0;
              cerr = fgets(ctmp2, D_CharTmpReadDef, fp);
              sscanf(ctmp2, "%d %lf %lf\n", &itmp, &dReValue, &dImValue);
              ParaQPTrans[itmp] = dReValue + I * dImValue;
            }
            idx = 0;
            if (APFlag == 0) {
              while (fscanf(fp, "%d %d ", &i, &j) != EOF) {
                ierr = fscanf(fp, "%d\n", &(QPTrans[i][j]));
                QPTransSgn[i][j] = 1;
                QPTransInv[i][QPTrans[i][j]] = j;
                idx++;
              }
            } else { /* anti-periodic boundary mode */
              while (fscanf(fp, "%d %d ", &i, &j) != EOF) {
                ierr = fscanf(fp, "%d %d\n", &(QPTrans[i][j]), &(QPTransSgn[i][j]));
                QPTransInv[i][QPTrans[i][j]] = j;
                idx++;
              }
            }
            if (idx != NQPTrans * Nsite) info = ReadDefFileError(defname);
          }
          fclose(fp);
          break;

        case KWOneBodyG:
          /*cisajs.def----------------------------------------*/
          if (NCisAjs > 0) {
            idx = 0;
            while (fscanf(fp, "%d %d %d %d\n",
                          &(x0), &(x1), &(x2), &(x3)) != EOF) {
              if(NLanczosMode <2) {
                CisAjsIdx[idx][0] = x0;
                CisAjsIdx[idx][1] = x1;
                CisAjsIdx[idx][2] = x2;
                CisAjsIdx[idx][3] = x3;
              }
              else {
                isite1=x0+x1*Nsite;
                isite2=x2+x3*Nsite;
                idxLanczos=iOneBodyGIdx[isite1][isite2];
                CisAjsIdx[idxLanczos][0] = x0;
                CisAjsIdx[idxLanczos][1] = x1;
                CisAjsIdx[idxLanczos][2] = x2;
                CisAjsIdx[idxLanczos][3] = x3;
                //fprintf(stdout, "Debug 1G: idxLanczos=%d\n", idxLanczos);
              }
              
              if (CheckPairSite(x0, x2, Nsite) != 0) {
                fprintf(stderr, "Error: Site index is incorrect. \n");
                info = 1;
                break;
              }

          /* OrbitalIdx?
          if(idx0!=(2*Nsite*Nsite-Nsite) || idx1!=NOrbitalIdx || itmp==1) {
            //printf("DEBUG: idx0=%d idx1=%d itmp = %d \n",idx0,idx1,itmp);
            info=ReadDefFileError(defname);
          }
          */
              idx++;
            }            
            if (NLanczosMode <2) {
              if (idx != NCisAjs) info = ReadDefFileError(defname);
            }
            else{
              if (idx != NCisAjsLz) info = ReadDefFileError(defname);
            }
          }
          fclose(fp);
          break;


        case KWTwoBodyGEx:
          /*cisajscktalt.def----------------------------------*/
          
          if (NCisAjsCktAlt > 0) {
            idx = 0;
            while (fscanf(fp, "%d %d %d %d %d %d %d %d\n",
                          &(x0), &(x1), &(x2), &(x3), &(x4),
                          &(x5), &(x6), &(x7)) != EOF) {
              CisAjsCktAltIdx[idx][0] = x0;
              CisAjsCktAltIdx[idx][1] = x1;
              CisAjsCktAltIdx[idx][2] = x2;
              CisAjsCktAltIdx[idx][3] = x3;
              CisAjsCktAltIdx[idx][4] = x4;
              CisAjsCktAltIdx[idx][5] = x5;
              CisAjsCktAltIdx[idx][6] = x6;
              CisAjsCktAltIdx[idx][7] = x7;
              if (CheckQuadSite(x0, x2, x4, x6, Nsite) != 0) {
                fprintf(stderr, "Error: Site index is incorrect. \n");
                info = 1;
                break;
              }
              idx++;
            }
            if (idx != NCisAjsCktAlt) info = ReadDefFileError(defname);
          }
          fclose(fp);
          
          break;

        case KWTwoBodyG:
          /*cisajscktaltdc.def--------------------------------*/
          if (NCisAjsCktAltDC > 0) {
            idx = 0;
            while (fscanf(fp, "%d %d %d %d %d %d %d %d\n",
                          &(x0), &(x1), &(x2), &(x3), &(x4),
                          &(x5), &(x6), &(x7)) != EOF) {
              CisAjsCktAltDCIdx[idx][0] = x0;
              CisAjsCktAltDCIdx[idx][1] = x1;
              CisAjsCktAltDCIdx[idx][2] = x2;
              CisAjsCktAltDCIdx[idx][3] = x3;
              CisAjsCktAltDCIdx[idx][4] = x4;
              CisAjsCktAltDCIdx[idx][5] = x5;
              CisAjsCktAltDCIdx[idx][6] = x6;
              CisAjsCktAltDCIdx[idx][7] = x7;
              
              if(NLanczosMode>1){
                isite1=x0+x1*Nsite;
                isite2=x2+x3*Nsite;
                idxLanczos=iOneBodyGIdx[isite1][isite2];
                CisAjsIdx[idxLanczos][0] = x0;
                CisAjsIdx[idxLanczos][1] = x1;
                CisAjsIdx[idxLanczos][2] = x2;
                CisAjsIdx[idxLanczos][3] = x3;
                CisAjsCktAltLzIdx[idx][0] = iOneBodyGIdx[isite1][isite2];
                //fprintf(stdout, "Debug 2G-1: idxLanczos=%d, x0=%d, x1=%d, x2=%d, x3=%d\n", idxLanczos, x0, x1, x2, x3);
                isite1=x4+x5*Nsite;
                isite2=x6+x7*Nsite;
                idxLanczos=iOneBodyGIdx[isite1][isite2];
                CisAjsIdx[idxLanczos][0] = x4;
                CisAjsIdx[idxLanczos][1] = x5;
                CisAjsIdx[idxLanczos][2] = x6;
                CisAjsIdx[idxLanczos][3] = x7;
                CisAjsCktAltLzIdx[idx][1] = iOneBodyGIdx[isite1][isite2];
                //fprintf(stdout, "Debug 2G-2: idxLanczos=%d, x4=%d, x5=%d, x6=%d, x7=%d\n", idxLanczos, x4, x5, x6, x7);
              }
               
              idx++;
              if (CheckQuadSite(x0, x2, x4, x6, Nsite) != 0) {
                fprintf(stderr, "Error: Site index is incorrect. \n");
                info = 1;
                break;
              }

              if (x1 != x3 || x5 != x7) {
                fprintf(stderr, "  Error:  Sz non-conserved system is not yet supported in mVMC ver.1.0.\n");
                info = ReadDefFileError(defname);
                break;
              }
            }
            if (idx != NCisAjsCktAltDC) info = ReadDefFileError(defname);
          }
          fclose(fp);
          break;

        case KWInterAll:
          /*interall.def---------------------------------------*/
          if (NInterAll > 0) {
            idx = 0;
            while (fscanf(fp, "%d %d %d %d %d %d %d %d %lf %lf\n",
                          &x0, &x1, &x2, &x3, &x4, &x5, &x6, &x7,
                          &dReValue, &dImValue) != EOF) {
              InterAll[idx][0] = x0;
              InterAll[idx][1] = x1;
              InterAll[idx][2] = x2;
              InterAll[idx][3] = x3;
              InterAll[idx][4] = x4;
              InterAll[idx][5] = x5;
              InterAll[idx][6] = x6;
              InterAll[idx][7] = x7;

              if (CheckQuadSite(x0, x2, x4, x6, Nsite) != 0) {
                fprintf(stderr, "Error: Site index is incorrect. \n");
                info = 1;
                break;
              }

              ParaInterAll[idx] = dReValue + I * dImValue;

              if (!((InterAll[idx][1] == InterAll[idx][3]
                     || InterAll[idx][5] == InterAll[idx][7])
                    ||
                    (InterAll[idx][1] == InterAll[idx][5]
                     || InterAll[idx][3] == InterAll[idx][7])
              )
                      ) {
                fprintf(stderr, "  Error:  Sz non-conserved system is not yet supported in mVMC ver.1.0.\n");
                info = ReadDefFileError(defname);
                break;
              }
              idx++;
            }
            if (idx != NInterAll) info = ReadDefFileError(defname);
          } else {
            /* do not terminate */
            /* info=ReadDefFileError(xNameListFile); */
          }
          fclose(fp);
          break;

        case KWOptTrans:
          /*qpopttrans.def------------------------------------*/
          if (FlagOptTrans > 0) {
            fidx = NProj + NOrbitalIdx;
            for (i = 0; i < NQPOptTrans; i++) {
              ierr = fscanf(fp, "%d ", &itmp);
              ierr = fscanf(fp, "%lf\n", &(ParaQPOptTrans[itmp]));
              OptFlag[fidx] = 1;
              fidx++;
              count_idx++;
            }
            idx = 0;
            if (APFlag == 0) {
              while (fscanf(fp, "%d %d ", &i, &j) != EOF) {
                ierr = fscanf(fp, "%d\n", &(QPOptTrans[i][j]));
                QPOptTransSgn[i][j] = 1;
                idx++;
              }
            } else { // anti-periodic boundary mode
              while (fscanf(fp, "%d %d ", &i, &j) != EOF) {
                ierr = fscanf(fp, "%d %d\n", &(QPOptTrans[i][j]), &(QPOptTransSgn[i][j]));
                idx++;
              }
            }
          }

          fclose(fp);
          break;

        case KWBFRange:
          /*rangebf.def--------------------------*/
          if (Nrange > 0) {
            for (i = 0; i < 5; i++) cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            idx0 = idx1 = 0;
            while (fscanf(fp, "%d %d %d\n", &i, &j, &n) != EOF) {
              PosBF[i][idx0 % Nrange] = j;
              RangeIdx[i][j] = n;
              //printf("PosBF[%d][%d]=%d\n",i,idx0 % Nrange, PosBF[i][idx0 % Nrange]);
              //if(globalrank==0)printf("RangeIdx[%d][%d]=%d\n",i,j, RangeIdx[i][j]);
              idx0++;
              if (idx0 == Nsite * Nrange) break;
            }
            if (idx0 != Nsite * Nrange) {
              info = ReadDefFileError(defname);
            }
          }
          fclose(fp);
          break;

        case KWBF:
          if (NBackFlowIdx > 0) {
            for (i = 0; i < 5; i++) cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
            idx0 = idx1 = 0;
            for (i = 0; i < Nsite * Nsite; i++) {
              for (j = 0; j < Nsite * Nsite; j++) {
                BackFlowIdx[i][j] = -1;
              }
            }
            while (fscanf(fp, "%d %d %d %d %d\n", &i, &j, &(x0), &(x1), &n) != EOF) {
              BackFlowIdx[i * Nsite + j][x0 * Nsite + x1] = n;
              idx0++;
              //printf("idx0=%d, idx=%d\n",idx0,BackFlowIdx[i]);
              if (idx0 == Nsite * Nsite * Nrange * Nrange) break;
            }
            while (fscanf(fp, "%d ", &i) != EOF) {
              ierr = fscanf(fp, "%d\n", &(OptFlag[fidx]));
              //printf("idx1=%d, OptFlag=%d\n",idx1,OptFlag[fidx]);
              fidx++;
              idx1++;
            }
            if (idx0 != Nsite * Nsite * Nrange * Nrange
                || idx1 != NBFIdxTotal * NBackFlowIdx) {
              info = ReadDefFileError(defname);
            }
          }
          fclose(fp);
          break;

        default:
          fclose(fp);
          break;
      }
    }

    if (count_idx != NPara) {
      fprintf(stderr, "error: OptFlag is incomplete.\n");
      info = 1;
    }
  } /* if(rank==0) */

  if(FlagOptTrans<=0){
    ParaQPOptTrans[0]=1.0;
    for(i=0;i<Nsite;++i) {
      QPOptTrans[0][i] = i;
      QPOptTransSgn[0][i] = 1;
    }
  }  
  if(info!=0) {
    if(rank==0) {
      fprintf(stderr, "error: Indices and Parameters of Definition files(*.def) are incomplete.\n");
    }
    MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
  }

#ifdef _mpi_use
  SafeMpiBcastInt(LocSpn, NTotalDefInt, comm);
  if(NLanczosMode>1) {
    SafeMpiBcastInt(CisAjsCktAltLzIdx[0], NCisAjsCktAltDC * 2, comm);
  }
  SafeMpiBcast_fcmp(ParaTransfer, NTransfer+NInterAll, comm);
  SafeMpiBcast(ParaCoulombIntra, NTotalDefDouble, comm);
  SafeMpiBcast_fcmp(ParaQPTrans, NQPTrans, comm);
#endif /* _mpi_use */
  
  /* set FlagShift */
  if(NVMCCalMode==0) {
    SetFlagShift();
    if(rank==0 && FlagShiftGJ+FlagShiftDH2+FlagShiftDH4>0 ) {
      fprintf(stderr, "remark: FlagShift ( ");
      if(FlagShiftGJ==1)  fprintf(stderr, "GJ ");
      if(FlagShiftDH2==1) fprintf(stderr, "DH2 ");
      if(FlagShiftDH4==1) fprintf(stderr, "DH4 ");
      fprintf(stderr, ") is turned on.\n");
    }
  }
  
  return 0;
}

int ReadInputParameters(char *xNameListFile, MPI_Comm comm)
{
  FILE *fp;
  char defname[D_FileNameMax];
  char ctmp[D_FileNameMax], ctmp2[256];
  int iKWidx=0;
  int i,idx;
  int rank;
  int count=0;
  int info=0;
  double tmp_real, tmp_comp;
  MPI_Comm_rank(comm, &rank);
  char *cerr;
  int ierr;
  
  if(rank==0) {
    for(iKWidx=KWInGutzwiller; iKWidx< KWIdxInt_end; iKWidx++){     
      strcpy(defname, cFileNameListFile[iKWidx]);
      if(strcmp(defname,"")==0) continue;   
      fp = fopen(defname, "r");
      if(fp==NULL){
        info= ReadDefFileError(defname);
        fclose(fp);
        continue;
      }
      /*=======================================================================*/
      cerr = fgets(ctmp, sizeof(ctmp)/sizeof(char), fp);//1
      cerr = fgets(ctmp2, sizeof(ctmp2)/sizeof(char), fp);//2
      sscanf(ctmp2, "%s %d\n", ctmp, &idx);
      cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);//3
      cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);//4
      cerr = fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);//5

		switch(iKWidx){
        //get idx

      case KWInGutzwiller:
        if(idx != NGutzwillerIdx){
          info=1;
          continue;
        }
        for(i=0; i<NGutzwillerIdx; i++){
          ierr = fscanf(fp, "%d %lf %lf ", &idx, &tmp_real,&tmp_comp);
          Proj[i]=tmp_real+I*tmp_comp;
        }
        break;
        
      case KWInJastrow:
        if(idx != NJastrowIdx){
          info=1;
          continue;
        }
        count= NGutzwillerIdx;
        for(i=count; i<count+NJastrowIdx; i++){
          ierr = fscanf(fp, "%d %lf %lf ", &idx, &tmp_real,&tmp_comp);
          Proj[i]=tmp_real+I*tmp_comp;
        }
        break;
        
      case KWInDH2:
        if(idx != NDoublonHolon2siteIdx){
          info=1;
          continue;
        }
        count =NGutzwillerIdx+NJastrowIdx;
        for(i=count; i<count+2*3*NDoublonHolon2siteIdx; i++){
          ierr = fscanf(fp, "%d %lf %lf ", &idx, &tmp_real,&tmp_comp);
          Proj[i]=tmp_real+I*tmp_comp;
        }       
        break;
        
      case KWInDH4:
        if(idx != NDoublonHolon4siteIdx){
          info=1;
          continue;
        }
        count =NGutzwillerIdx+NJastrowIdx+2*3*NDoublonHolon2siteIdx;
        for(i=count; i<count+2*5*NDoublonHolon4siteIdx; i++){
          ierr = fscanf(fp, "%d %lf %lf ", &idx, &tmp_real,&tmp_comp);
          Proj[i]=tmp_real+I*tmp_comp;
        }        
        break;
        
      case KWInOrbital:
        if(idx != NOrbitalIdx){
          info=1;
          continue;
        }
        for(i=0; i<NSlater; i++){
          ierr = fscanf(fp, "%d %lf %lf ", &idx, &tmp_real,&tmp_comp);
          Slater[i]=tmp_real+I*tmp_comp;
        }  
        break;
        
      case KWInOptTrans:
        if(idx != NOptTrans){
          info=1;
          continue;
        }
        for(i=0; i<NOptTrans; i++){
          ierr = fscanf(fp, "%d %lf %lf ", &idx, &tmp_real,&tmp_comp);
          OptTrans[i]=tmp_real+I*tmp_comp;
        }  
        break;
        
      default:
        break;
      }
      fclose(fp);
    }
  } /* if(rank==0) */

  if(info!=0) {
    if(rank==0) {
      fprintf(stderr, "error: Indices of %s file is incomplete.\n", defname);
    }
    MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
  }  
  return 0;
}



/**********************************************************************/
/* Function checking keywords*/
/**********************************************************************/
/** 
 * 
 * @brief function of checking whether ctmp is same as cKeyWord or not
 * 
 * @param[in] ctmp 
 * @param[in] cKeyWord 
 * @return 0 ctmp is same as cKeyWord
 * 
 * @author Kazuyoshi Yoshimi (The University of Tokyo)
 */
int CheckWords(
	       const char* ctmp,
	       const char* cKeyWord
	       )
{

  int i=0;

  char ctmp_small[256]={0};
  char cKW_small[256]={0};
  int n;
  n=strlen(cKeyWord);
  strncpy(cKW_small, cKeyWord, n);
  
  for(i=0; i<n; i++){
    cKW_small[i]=tolower(cKW_small[i]);
  }
  
  n=strlen(ctmp);
  strncpy(ctmp_small, ctmp, n);
  for(i=0; i<n; i++){
    ctmp_small[i]=tolower(ctmp_small[i]);
  }
  if(n<strlen(cKW_small)) n=strlen(cKW_small);
  return(strncmp(ctmp_small, cKW_small, n));
}

/**
 * @brief Check Site Number.
 * @param[in] *iSite a site number.
 * @param[in] iMaxNum Max site number.
 * @retval 0 normally finished reading file.
 * @retval -1 unnormally finished reading file.
 * @version 0.1
 * @author Takahiro Misawa (The University of Tokyo)
 * @author Kazuyoshi Yoshimi (The University of Tokyo)
 **/
int CheckSite(
        const int iSite,
        const int iMaxNum
)
{
    if(iSite>=iMaxNum || iSite < 0)  return(-1);
    return 0;
}

/**
 * @brief Check Site Number for a pair -> (siteA, siteB).
 * @param[in] iSite1 a site number on a site A.
 * @param[in] iSite2 a site number on a site B.
 * @param[in] iMaxNum Max site number.
 * @retval 0 normally finished reading file.
 * @retval -1 unnormally finished reading file.
 * @version 0.1
 * @author Takahiro Misawa (The University of Tokyo)
 * @author Kazuyoshi Yoshimi (The University of Tokyo)
 **/
int CheckPairSite(
        const int iSite1,
        const int iSite2,
        const int iMaxNum
)
{
    if(CheckSite(iSite1, iMaxNum)!=0){
        return(-1);
    }
    if(CheckSite(iSite2, iMaxNum)!=0){
        return(-1);
    }
    return 0;
}

/**
 * @brief Check Site Number for a quad -> (siteA, siteB, siteC, siteD).
 * @param[in] iSite1 a site number on site A.
 * @param[in] iSite2 a site number on site B.
 * @param[in] iSite3 a site number on site C.
 * @param[in] iSite4 a site number on site D.
 * @param[in] iMaxNum Max site number.
 * @retval 0 normally finished reading file.
 * @retval -1 unnormally finished reading file.
 * @version 0.1
 * @author Takahiro Misawa (The University of Tokyo)
 * @author Kazuyoshi Yoshimi (The University of Tokyo)
 **/
int CheckQuadSite(
        const int iSite1,
        const int iSite2,
        const int iSite3,
        const int iSite4,
        const int iMaxNum
)
{
    if(CheckPairSite(iSite1, iSite2, iMaxNum)!=0){
        return(-1);
    }
    if(CheckPairSite(iSite3, iSite4, iMaxNum)!=0){
        return(-1);
    }
    return 0;
}


/**
 * @brief Function of Checking keyword in NameList file.
 * @param[in] _cKW keyword candidate
 * @param[in] _cKWList Reffercnce of keyword List
 * @param[in] iSizeOfKWidx number of keyword
 * @param[out] _iKWidx index of keyword
 * @retval 0 keyword is correct.
 * @retval -1 keyword is incorrect.
 * @version 0.1
 * @author Takahiro Misawa (The University of Tokyo)
 * @author Kazuyoshi Yoshimi (The University of Tokyo)
 **/
int CheckKW(
	    const char* cKW,
	    char  cKWList[][D_CharTmpReadDef],
	    int iSizeOfKWidx,
	    int* iKWidx
	    ){
  *iKWidx=-1;
  int itmpKWidx;
  int iret=-1;
  for(itmpKWidx=0; itmpKWidx<iSizeOfKWidx; itmpKWidx++){
    if(strcmp(cKW,"")==0){
      break;
    }
    else if(CheckWords(cKW, cKWList[itmpKWidx])==0){
      iret=0;
      *iKWidx=itmpKWidx;
    }
  }
  return iret;
}

/**
 * @brief Function of Validating value.
 * @param[in] icheckValue value to validate.
 * @param[in] ilowestValue lowest value which icheckValue can be set.
 * @param[in] iHighestValue heighest value which icheckValue can be set.
 * @retval 0 value is correct.
 * @retval -1 value is incorrect.
 * @author Kazuyoshi Yoshimi (The University of Tokyo)
 **/
int ValidateValue(
		  const int icheckValue, 
		  const int ilowestValue, 
		  const int iHighestValue
		  ){

  if(icheckValue < ilowestValue || icheckValue > iHighestValue){
    return(-1);
  }
  return 0;
}

/**
 * @brief Function of Getting keyword and it's variable from characters.
 * @param[in] _ctmpLine characters including keyword and it's variable 
 * @param[out] _ctmp keyword
 * @param[out] _itmp variable for a keyword
 * @retval 0 keyword and it's variable are obtained.
 * @retval 1 ctmpLine is a comment line.
 * @retval -1 format of ctmpLine is incorrect.
 * @version 1.0
 * @author Kazuyoshi Yoshimi (The University of Tokyo)
 **/
int GetKWWithIdx(
		 char *ctmpLine,
		 char *ctmp,
		 int *itmp
		 )
{
  char *ctmpRead;
  char *cerror;
  char csplit[] = " ,.\t\n";
  if(*ctmpLine=='\n') return(-1);
  ctmpRead = strtok(ctmpLine, csplit);
  if(strncmp(ctmpRead, "=", 1)==0 || strncmp(ctmpRead, "#", 1)==0 || ctmpRead==NULL){
    return(-1);
  }
  strcpy(ctmp, ctmpRead);
    
  ctmpRead = strtok( NULL, csplit );
  *itmp = strtol(ctmpRead, &cerror, 0);
  //if ctmpRead is not integer type
  if(*cerror != '\0'){
    fprintf(stderr, "Error: incorrect format= %s. \n", cerror);
    return(-1);
  }

  ctmpRead = strtok( NULL, csplit );
  if(ctmpRead != NULL){
    fprintf(stderr, "Error: incorrect format= %s. \n", ctmpRead);
    return(-1);
  }    
  return 0;
}

/**
 * @brief Function of Fitting FileName
 * @param[in]  _cFileListNameFile file for getting names of input files.
 * @param[out] _cFileNameList arrays for getting names of input files.
 * @retval 0 normally finished reading file.
 * @retval -1 unnormally finished reading file.
 * @version 1.0
 * @author Kazuyoshi Yoshimi (The University of Tokyo)
 **/
int GetFileName(
		const char* cFileListNameFile,
		char cFileNameList[][D_CharTmpReadDef]
		)
{
  FILE *fplist;
  int itmpKWidx=-1;
  char ctmpFileName[D_FileNameMaxReadDef];
  char ctmpKW[D_CharTmpReadDef], ctmp2[256];
  int i;
  for(i=0; i< KWIdxInt_end; i++){
    strcpy(cFileNameList[i],"");
  }
  
  fplist = fopen(cFileListNameFile, "r");
  if(fplist==NULL) return ReadDefFileError(cFileListNameFile);
  
  while(fgets(ctmp2, 256, fplist) != NULL){ 
    //memset(ctmpKW, '\0', strlen(ctmpKW));
    //memset(ctmpFileName, '\0', strlen(ctmpFileName));
        sscanf(ctmp2,"%s %s\n", ctmpKW, ctmpFileName);
		if(strncmp(ctmpKW, "#", 1)==0 || *ctmp2=='\n' || (strcmp(ctmpKW, "")&&strcmp(ctmpFileName,""))==0)
        {
			continue;
		}
    else if(strcmp(ctmpKW, "")*strcmp(ctmpFileName, "")==0){
      fprintf(stderr, "Error: keyword and filename must be set as a pair in %s.\n", cFileListNameFile);
      fclose(fplist);
      return(-1);
    }
    /*!< Check KW */
    if( CheckKW(ctmpKW, cKWListOfFileNameList, KWIdxInt_end, &itmpKWidx)!=0 ){

      fprintf(stderr, "Error: Wrong keywords '%s' in %s.\n", ctmpKW, cFileListNameFile);
      fprintf(stderr, "%s", "Choose Keywords as follows: \n");
      for(i=0; i<KWIdxInt_end;i++){
        fprintf(stderr, "%s \n", cKWListOfFileNameList[i]);
      }      
      fclose(fplist);
      return(-1);
    }
    /*!< Check cFileNameList to prevent from double registering the file name */    
    if(strcmp(cFileNameList[itmpKWidx], "") !=0){
      fprintf(stderr, "Error: Same keywords exist in %s.\n", cFileListNameFile);
      fclose(fplist);
      return(-1);
    }
    /*!< Copy FileName */
    strcpy(cFileNameList[itmpKWidx], ctmpFileName);
  }
  fclose(fplist);  
  return 0;
}

void SetDefultValuesModPara(int *bufInt, double* bufDouble){
  
  bufInt[IdxVMCCalcMode]=0;
  bufInt[IdxLanczosMode]=0;
  bufInt[IdxDataIdxStart]=0;
  bufInt[IdxDataQtySmp]=1;
  bufInt[IdxNsite]=16;
  bufInt[IdxNe]=8;
  bufInt[IdxSPGaussLeg]=8;
  bufInt[IdxSPStot]=0;
  bufInt[IdxMPTrans]=0;
  bufInt[IdxSROptItrStep]=1000;
  bufInt[IdxSROptItrSmp]=bufInt[IdxSROptItrStep]/10;
  bufInt[IdxSROptFixSmp]=1;
  bufInt[IdxVMCWarmUp]=10;
  bufInt[IdxVMCInterval]=1;
  bufInt[IdxVMCSample]=10;
  bufInt[IdxExUpdatePath]=0;
  bufInt[IdxRndSeed]=11272;
  bufInt[IdxSplitSize]=1;
  bufInt[IdxNLocSpin]=0;
  bufInt[IdxNTrans]=0;
  bufInt[IdxNCoulombIntra]=0;
  bufInt[IdxNCoulombInter]=0;
  bufInt[IdxNHund]=0;
  bufInt[IdxNPairHop]=0;
  bufInt[IdxNExchange]=0;
  bufInt[IdxNGutz]=0;
  bufInt[IdxNJast]=0;
  bufInt[IdxNDH2]=0;
  bufInt[IdxNDH4]=0;
  bufInt[IdxNOrbit]=0;
  bufInt[IdxNOrbitAntiParallel]=0;
  bufInt[IdxNOrbitParallel]=0;
  bufInt[IdxNQPTrans]=0;
  bufInt[IdxNOneBodyG]=0;
  bufInt[IdxNTwoBodyG]=0;
  bufInt[IdxNTwoBodyGEx]=0;
  bufInt[IdxNInterAll]=0;
  bufInt[IdxNQPOptTrans]=1;
  bufInt[IdxNBF]=0;
  bufInt[IdxNrange]=0;
  bufInt[IdxNNz]=0;

  bufDouble[IdxSROptRedCut]=0.001;
  bufDouble[IdxSROptStaDel]=0.02;
  bufDouble[IdxSROptStepDt]=0.02;
  NStoreO=1;
}

/**********************************************************************/
