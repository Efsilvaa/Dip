//===========================================================================//
// This file is part of the DIP Solver Framework.                            //
//                                                                           //
// DIP is distributed under the Eclipse Public License as part of the        //
// COIN-OR repository (http://www.coin-or.org).                              //
//                                                                           //
// Author: Matthew Galati, SAS Institute Inc. (matthew.galati@sas.com)       //
//                                                                           //
// Conceptual Design: Matthew Galati, SAS Institute Inc.                     //
//                    Ted Ralphs, Lehigh University                          //
//                                                                           //
// Conceptual Design: Matthew Galati, SAS Institute Inc.                     //
//                    Ted Ralphs, Lehigh University                          //
//                                                                           //
// Copyright (C) 2002-2009, Lehigh University, Matthew Galati, Ted Ralphs    //
// All Rights Reserved.                                                      //
//===========================================================================//

//===========================================================================//
#include "UtilHash.h"
#include "UtilMacrosDecomp.h"
#include "DecompConstraintSet.h"

//===========================================================================//
void DecompConstraintSet::prepareModel(){
   //---
   //--- For each model:
   //---   1.) set row senses and/or bounds
   //---   2.) create row hash
   //---   3.) set nBaseRows
   //---   4.) flip to row ordered, if neccessary (for relaxed too)
   //--- TODO: put timer on this ??
   //---
   if(!M)
      return;
   if(M->isColOrdered())
      M->reverseOrdering();
   
   checkSenseAndBound();    
   createRowHash();//TODO: don't need for relaxed
   nBaseRows = getNumRows();

   //TODO: make this an option
   //---
   //--- if row/col names are not given, make up default ones
   //---
   int i, j;
   if(rowNames.size() == 0){
      for(i = 0; i < getNumRows(); i++)
         rowNames.push_back("r(" + UtilIntToStr(i) + ")");
   }
   if(colNames.size() == 0){
      for(j = 0; j < getNumCols(); j++)
         colNames.push_back("x(" + UtilIntToStr(j) + ")");
   }
   prepHasRun = true;

   //---
   //--- if active columns were not set, set to all columns
   //---
   int nActiveColumns = static_cast<int>(activeColumns.size());
   if(!nActiveColumns)
      UtilIotaN(activeColumns, getNumCols(), 0);

   //---
   //--- create set from vector - easier to check overlap, etc
   //---
   vector<int>::iterator vit;
   for(vit = activeColumns.begin(); vit != activeColumns.end(); vit++)
      activeColumnsS.insert(*vit);

   //---
   //--- set column markers
   //---
   UtilFillN(columnMarker, getNumCols(), (int)DecompColNonActive);
   for(vit = activeColumns.begin(); vit != activeColumns.end(); vit++)
      columnMarker[*vit] = DecompColActive;
   
   for(vit = masterOnlyCols.begin(); vit != masterOnlyCols.end(); vit++)
      columnMarker[*vit] = DecompColMasterOnly;   
}

//===========================================================================//
void DecompConstraintSet::createRowHash(){
  
  int    r;
  string strHash;
  
  const int    * rmat_ind = M->getIndices();
  const double * rmat_els = M->getElements();
  const int    * rmat_beg = M->getVectorStarts();
  const int    * rmat_len = M->getVectorLengths();
  for(r = 0; r < getNumRows(); r++){    
    strHash = UtilCreateStringHash(rmat_len[r],
                                   rmat_ind + rmat_beg[r],
                                   rmat_els + rmat_beg[r],
                                   rowSense[r],
                                   rowRhs[r]);
    rowHash.push_back(strHash);
  }
}

//===========================================================================//
void DecompConstraintSet::checkSenseAndBound(){
  assert(rowLB.size() + rowRhs.size() > 0);
  assert(rowUB.size() + rowRhs.size() > 0);
  
  if(rowLB.size() > 0 && rowRhs.size() == 0){
    boundsToSenses();
  }else if(rowLB.size() == 0 && rowRhs.size() > 0){
    sensesToBounds();
  }

  assert(rowLB.size() == rowUB.size());
  assert(rowLB.size() == rowRhs.size());
  assert(rowLB.size() == rowSense.size());
}

//===========================================================================//
void DecompConstraintSet::sensesToBounds(){
  double rlb, rub;
  int    n_rows = static_cast<int>(rowSense.size());
  rowLB.reserve(n_rows);
  rowUB.reserve(n_rows);
  for(int r = 0; r < n_rows; r++){
    UtilSenseToBound(rowSense[r], rowRhs[r], 0.0,//TODO
                     DecompInf, rlb, rub);
    rowLB.push_back(rlb);
    rowUB.push_back(rub);
  }
}

//===========================================================================//
void DecompConstraintSet::boundsToSenses(){
  char   sense;
  double rhs, range;//not used
  int    n_rows = static_cast<int>(rowLB.size());  
  rowRhs.reserve(n_rows);
  rowSense.reserve(n_rows);
  for(int r = 0; r < n_rows; r++){
    UtilBoundToSense(rowLB[r], rowUB[r], DecompInf, 
                     sense, rhs, range);
    rowRhs.push_back(rhs);
    rowSense.push_back(sense);
  }
}

//===========================================================================//
void DecompConstraintSet::fixNonActiveColumns(){
   const int numCols     = getNumCols();
   const int nActiveCols = static_cast<int>(activeColumns.size());
   if(nActiveCols == numCols)
      return;

   int * marker = new int[numCols];
   if(!marker)
      UtilExceptionMemory("fixNonActiveColumns", "DecompConstraintSet");
   UtilFillN(marker, numCols, 0);

   vector<int>::iterator vi;
   for(vi = activeColumns.begin(); vi != activeColumns.end(); vi++){
      marker[*vi] = 1;
   }

   int i;
   for(i = 0; i < numCols; i++){
      if(marker[i])
         continue;
      colLB[i] = 0.0;
      colUB[i] = 0.0;
   }

   UTIL_DELARR(marker);
}
