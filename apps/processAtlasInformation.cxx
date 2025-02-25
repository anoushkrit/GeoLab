///////////////////////////////////////////////////////////////////////////////
//---------------------------- Libraries ------------------------------------//
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <cmath>
#include <string.h>
#include <omp.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <numeric>
#include <chrono>
#include <omp.h>
#include <experimental/filesystem>

#include "processAtlasInformation.h"
#include "ioWrapper.h"
#include "niftiImage.h"

///////////////////////////////////////////////////////////////////////////////
////////// Function to get flag position when parsing arguments ///////////////
///////////////////////////////////////////////////////////////////////////////

int getFlagPosition( int argc, char* argv[], const std::string& flag )
{

  for ( int i = 0 ; i < argc ; i++ )
  {

    std::string arg = argv[ i ] ;
    // if ( 0 == arg.find( flag ) )
    if ( arg == flag )
    {

      return i ;

    }

  }
  return 0 ;

}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// Functions ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------//
//-------------------------- Find radius of bundle ---------------------------//
//----------------------------------------------------------------------------//
void computeCenterAtlasBundleFibers(
                             BundlesData& atlasBundleData,
                             std::vector<float>& medialPointsAtlasBundleFibers )
{

  int nbFibersAtlasBundle = atlasBundleData.curves_count ;

  #pragma omp parallel for
  for ( int atlasBundleFiberIndex = 0 ;
                             atlasBundleFiberIndex < nbFibersAtlasBundle ;
                                                      atlasBundleFiberIndex++ )
  {

    // Searching the medial point of atlas fiber
    std::vector<float> medialPointAtlasBundleFiber( 3, 0 ) ;
    atlasBundleData.computeMedialPointFiberWithDistance(
                                                 atlasBundleFiberIndex,
                                                 medialPointAtlasBundleFiber ) ;

    for ( int i = 0 ; i < 3 ; i++ )
    {

      medialPointsAtlasBundleFibers[ 3 * atlasBundleFiberIndex + i ] =
                                              medialPointAtlasBundleFiber[ i ] ;

    }

  }

}

//----------------------------------------------------------------------------//
//----------------------- Compute average fiber bundle -----------------------//
//----------------------------------------------------------------------------//
void computeAverageFiberBundle(
                        BundlesData& atlasBundleData,
                        const std::vector<float>& medialPointsAtlasBundleFibers,
                        int nbPoints,
                        std::vector<float>& averageFiber,
                        std::vector<float>& medialPointAtlasBundle )
{

  int nbFibersAtlasBundle = atlasBundleData.curves_count ;


  std::vector<float> referenceFiber( 3 * nbPoints, 0 ) ;
  int fiberIndex = 0 ;
  atlasBundleData.getFiberFromTractogram( atlasBundleData.matrixTracks,
                                          fiberIndex,
                                          nbPoints,
                                          referenceFiber ) ;



  std::vector<float> centerReferenceFiber( 3, 0 ) ;
  for ( int i = 0 ; i < 3 ; i++ )
  {

    centerReferenceFiber[ i ] = medialPointsAtlasBundleFibers[ 3 * 0 + i ] ;

  }


  // #pragma omp parallel for reduction( +:averageFiber[:nbElementsAverageFiber ] )
  for ( int atlasBundleFiberIndex = 0 ;
                             atlasBundleFiberIndex < nbFibersAtlasBundle ;
                                                      atlasBundleFiberIndex++ )
  {

    int offsetAtlas = 3 * nbPoints * atlasBundleFiberIndex ;

    std::vector<float> translation( 3, 0 ) ;
    for ( int i = 0 ; i < 3 ; i++ )
    {

      translation[ i ] = centerReferenceFiber[ i ] -
               medialPointsAtlasBundleFibers[ 3 * atlasBundleFiberIndex + i ] ;

    }

    float directDistance = 0 ;
    float indirectDistance = 0 ;
    for ( int i = 0 ; i < 3 ; i++ )
    {

      directDistance += pow( referenceFiber[ 3 * 0 + i ] - ( atlasBundleData[ 3
                             * 0 + offsetAtlas + i ] + translation[ i ] ), 2 ) ;
      indirectDistance += pow( referenceFiber[ 3 * 0 + i ] - ( atlasBundleData[
            3 * ( nbPoints - 1 ) + offsetAtlas + i ] + translation[ i ] ), 2 ) ;

    }

    bool isDirectSens = true ;
    if ( directDistance > indirectDistance )
    {

      isDirectSens = false ;

    }

    for ( int point = 0 ; point < nbPoints ; point++ )
    {

      int k = point ;
      if ( !isDirectSens )
      {

        k = nbPoints - point - 1 ;

      }

      for ( int i = 0 ; i < 3 ; i++ )
      {

        averageFiber[ 3 * point + i ] +=
                                    atlasBundleData[ 3 * k + offsetAtlas + i ] ;

      }

    }

  }


  for ( int point = 0 ; point < nbPoints ; point++ )
  {

    for ( int i = 0 ; i < 3 ; i++ )
    {

      averageFiber[ 3 * point + i ] /= nbFibersAtlasBundle ;

    }

  }



  // Searching the medial point of average atlas bundle fiber
  atlasBundleData.computeMedialPointFiberWithDistance(
                                                      averageFiber,
                                                      medialPointAtlasBundle ) ;

}


//----------------------------------------------------------------------------//
//--------------------- Compute gravity center of bundle ---------------------//
//----------------------------------------------------------------------------//
void computeGravityCenterAtlasBundle(
                                  BundlesData& atlasBundleData,
                                  int nbPoints,
                                  std::vector<float>& gravityCenterAtlasBundle )
{

  int nbFibersAtlasBundle = atlasBundleData.curves_count ;

  for ( int i = 0 ; i < 3 ; i++ )
  {

    gravityCenterAtlasBundle[ i ] = 0 ;

  }

  for ( int atlasBundleFiberIndex = 0 ;
                             atlasBundleFiberIndex < nbFibersAtlasBundle ;
                                                      atlasBundleFiberIndex++ )
  {

    int offsetAtlas = 3 * nbPoints * atlasBundleFiberIndex ;

    for ( int point = 0 ; point < nbPoints ; point++ )
    {

      for ( int i = 0 ; i < 3 ; i++ )
      {

        gravityCenterAtlasBundle[ i ] +=
                                    atlasBundleData[ 3 * point + offsetAtlas + i ] ;

      }

    }

  }

  for ( int i = 0 ; i < 3 ; i++ )
  {

    gravityCenterAtlasBundle[ i ] /= nbFibersAtlasBundle * nbPoints ;

  }

}


//----------------------------------------------------------------------------//
//-------------------------- Find radius of bundle ---------------------------//
//----------------------------------------------------------------------------//
void computeDistancesToCenterBundle(
                        const std::vector<float>& medialPointsAtlasBundleFibers,
                        const std::vector<float>& medialPointAtlasBundle,
                        int nbFibersAtlasBundle,
                        std::vector<float>& distancesToCenterAtlasBundle )
{

  #pragma omp parallel for
  for ( int atlasBundleFiberIndex = 0 ;
                             atlasBundleFiberIndex < nbFibersAtlasBundle ;
                                                      atlasBundleFiberIndex++ )
  {

    float distanceToCenter = 0 ;

    for ( int i = 0 ; i < 3 ; i++ )
    {

      distanceToCenter += pow( medialPointAtlasBundle[ i ] -
           medialPointsAtlasBundleFibers[ 3 * atlasBundleFiberIndex + i ], 2 ) ;


    }

    distanceToCenter = sqrt( distanceToCenter ) ;

    distancesToCenterAtlasBundle[ atlasBundleFiberIndex ] = distanceToCenter ;

  }

}


//----------------------------------------------------------------------------//
//------------- Compute normal vector of fibers in atlas bundle --------------//
//----------------------------------------------------------------------------//
void computeNormalVectorFibersAtlasBundle(
                                       BundlesData& atlasBundleData,
                                       std::vector<float>& normalVectorsBundle )
{

  int nbFibersAtlasBundle = atlasBundleData.curves_count ;

  #pragma omp parallel for
  for ( int atlasBundleFiberIndex = 0 ;
                             atlasBundleFiberIndex < nbFibersAtlasBundle ;
                                                      atlasBundleFiberIndex++ )
  {

    std::vector<float> normalVector( 3, 0 ) ;
    atlasBundleData.computeNormalVectorFiberTractogram( atlasBundleFiberIndex,
                                                        normalVector ) ;

    normalVectorsBundle[ 3 * atlasBundleFiberIndex + 0 ] = normalVector[ 0 ] ;
    normalVectorsBundle[ 3 * atlasBundleFiberIndex + 1 ] = normalVector[ 1 ] ;
    normalVectorsBundle[ 3 * atlasBundleFiberIndex + 2 ] = normalVector[ 2 ] ;


  }

}

//----------------------------------------------------------------------------//
//--------- Compute distances between medial points fibers in bundle ---------//
//----------------------------------------------------------------------------//
void computeDistancesBetweenMedialPointsBundle(
                        BundlesData& atlasBundleData,
                        const std::vector<float>& medialPointsAtlasBundleFibers,
                        std::vector<float>& distancesBetweenMedialPointsBundle )
{

  int nbFibersAtlasBundle = atlasBundleData.curves_count ;

  int indexDistanceBetweenMedialPointsBundle = 0 ;

  #pragma omp parallel for
  for ( int atlasBundleFiberIndex_1 = 0 ;
                             atlasBundleFiberIndex_1 < nbFibersAtlasBundle ;
                                                    atlasBundleFiberIndex_1++ )
  {

    std::vector<float> medialPointFiber1( 3, 0 ) ;
    for ( int i = 0 ; i < 3 ; i++ )
    {

      medialPointFiber1[ i ] = medialPointsAtlasBundleFibers[
                                             3 * atlasBundleFiberIndex_1 + i ] ;

    }

    if ( atlasBundleFiberIndex_1 + 1 < nbFibersAtlasBundle )
    {

      for ( int atlasBundleFiberIndex_2 = atlasBundleFiberIndex_1 + 1 ;
                                 atlasBundleFiberIndex_2 < nbFibersAtlasBundle ;
                                                     atlasBundleFiberIndex_2++ )
      {

        if ( atlasBundleFiberIndex_2 != atlasBundleFiberIndex_1 )
        {

          std::vector<float> medialPointFiber2( 3, 0 ) ;
          for ( int i = 0 ; i < 3 ; i++ )
          {

            medialPointFiber2[ i ] = medialPointsAtlasBundleFibers[
                                             3 * atlasBundleFiberIndex_2 + i ] ;

          }

          float tmpDistance = 0 ;

          for ( int i = 0 ; i < 3 ; i++ )
          {

            tmpDistance += pow( medialPointFiber1[ i ] -
                                                   medialPointFiber2[ i ], 2 ) ;


          }

          tmpDistance = sqrt( tmpDistance ) ;

          indexDistanceBetweenMedialPointsBundle = atlasBundleFiberIndex_1 *
                                                 ( 2 * nbFibersAtlasBundle - 1 -
                                                 atlasBundleFiberIndex_1 ) / 2 +
                                                 atlasBundleFiberIndex_2 -
                                                   atlasBundleFiberIndex_1 - 1 ;


          distancesBetweenMedialPointsBundle[
                        indexDistanceBetweenMedialPointsBundle ] = tmpDistance ;

        }

      }

    }

  }

}


//----------------------------------------------------------------------------//
//--------------------------- Compute angle bundle ---------------------------//
//----------------------------------------------------------------------------//
void computeAnglesBundle( BundlesData& atlasBundleData,
                          const std::vector<float>& normalVectorsBundle,
                          std::vector<float>& anglesAtlasBundle )
{

  int nbFibersAtlasBundle = atlasBundleData.curves_count ;

  int indexAnglesAtlasBundle = 0 ;

  #pragma omp parallel for
  for ( int atlasBundleFiberIndex_1 = 0 ;
                             atlasBundleFiberIndex_1 < nbFibersAtlasBundle ;
                                                    atlasBundleFiberIndex_1++ )
  {

    std::vector<float> normalVectorBundleFiber_1( 3, 0 ) ;
    for ( int i = 0 ; i < 3 ; i++ )
    {

      normalVectorBundleFiber_1[ i ] =
                      normalVectorsBundle[ 3 * atlasBundleFiberIndex_1 + i ] ;

    }

    if ( atlasBundleFiberIndex_1 + 1 < nbFibersAtlasBundle )
    {

      for ( int atlasBundleFiberIndex_2 = atlasBundleFiberIndex_1 + 1 ;
                                 atlasBundleFiberIndex_2 < nbFibersAtlasBundle ;
                                                     atlasBundleFiberIndex_2++ )
      {

        if ( atlasBundleFiberIndex_2 != atlasBundleFiberIndex_1 )
        {

          std::vector<float> normalVectorBundleFiber_2( 3, 0 ) ;
          for ( int i = 0 ; i < 3 ; i++ )
          {

            normalVectorBundleFiber_2[ i ] =
                        normalVectorsBundle[ 3 * atlasBundleFiberIndex_2 + i ] ;

          }

          float angle = atlasBundleData.computeAngleBetweenVectors(
                                                   normalVectorBundleFiber_1,
                                                   normalVectorBundleFiber_2 ) ;

          indexAnglesAtlasBundle = atlasBundleFiberIndex_1 *
                                                 ( 2 * nbFibersAtlasBundle - 1 -
                                                 atlasBundleFiberIndex_1 ) / 2 +
                                                 atlasBundleFiberIndex_2 -
                                                   atlasBundleFiberIndex_1 - 1 ;

          if ( angle > 90 )
          {

            angle = 180 - angle ;

          }

          anglesAtlasBundle[ indexAnglesAtlasBundle ] = angle ;

        }

      }

    }

  }

}


//----------------------------------------------------------------------------//
// --------------------- Compute direction angles bundle -------------------- //
//----------------------------------------------------------------------------//

// void computeDirectionAnglesBundle(
//                                 BundlesData& atlasBundleData,
//                                 const std::vector<float>& normalVectorsBundle,
//                                 int nbPoints,
//                                 std::vector<float>& directionAnglesAtlasBundle )
// {
//
//   int nbFibersAtlasBundle = atlasBundleData.curves_count ;
//
//   int indexAnglesAtlasBundle = 0 ;
//
//   #pragma omp parallel for
//   for ( int atlasBundleFiberIndex_1 = 0 ;
//                              atlasBundleFiberIndex_1 < nbFibersAtlasBundle ;
//                                                     atlasBundleFiberIndex_1++ )
//   {
//
//     std::vector<float> normalVector1( 3, 0 ) ;
//     for ( int i =0 ; i < 3 ; i++ )
//     {
//
//       normalVector1[ i ] = normalVectorsBundle[
//                                              3 * atlasBundleFiberIndex_1 + i ] ;
//
//     }
//
//     std::vector<float> fiber1( 3 * nbPoints, 0 ) ;
//     std::vector<float> newNormalVectorFiber1( 3, 0 ) ;
//     atlasBundleData.putFiberInPlaneXY( normalVector1,
//                                        atlasBundleData.matrixTracks,
//                                        atlasBundleFiberIndex_1,
//                                        nbPoints,
//                                        fiber1,
//                                        newNormalVectorFiber1 ) ;
//
//     std::vector<float> directionVectorFiber1( 3, 0 ) ;
//     atlasBundleData.computeDirectionVectorFiberTractogram(
//                                                        fiber1,
//                                                        newNormalVectorFiber1,
//                                                        directionVectorFiber1 ) ;
//
//
//     if ( atlasBundleFiberIndex_1 + 1 < nbFibersAtlasBundle )
//     {
//
//       for ( int atlasBundleFiberIndex_2 = atlasBundleFiberIndex_1 + 1 ;
//                                  atlasBundleFiberIndex_2 < nbFibersAtlasBundle ;
//                                                       atlasBundleFiberIndex_2++ )
//       {
//
//         if ( atlasBundleFiberIndex_2 != atlasBundleFiberIndex_1 )
//         {
//
//           std::vector<float> normalVector2( 3, 0 ) ;
//           for ( int i =0 ; i < 3 ; i++ )
//           {
//
//             normalVector2[ i ] = normalVectorsBundle[
//                                              3 * atlasBundleFiberIndex_2 + i ] ;
//
//           }
//
//           std::vector<float> fiber2( 3 * nbPoints, 0 ) ;
//           std::vector<float> newNormalVectorFiber2( 3, 0 ) ;
//           atlasBundleData.putFiberInPlaneXY( normalVector2,
//                                              atlasBundleData.matrixTracks,
//                                              atlasBundleFiberIndex_2,
//                                              nbPoints,
//                                              fiber2,
//                                              newNormalVectorFiber2 ) ;
//
//           std::vector<float> directionVectorFiber2( 3, 0 ) ;
//           atlasBundleData.computeDirectionVectorFiberTractogram(
//                                                        fiber2,
//                                                        newNormalVectorFiber2,
//                                                        directionVectorFiber2 ) ;
//
//
//           float angleBetweenPlanes = atlasBundleData.computeAngleBetweenPlanes(
//                                                        newNormalVectorFiber1,
//                                                        newNormalVectorFiber2 ) ;
//
//           if ( angleBetweenPlanes > 5 )
//           {
//
//             std::cout << "\nERROR : could not align fibers, got minimum angle "
//                       << "between planes of " << angleBetweenPlanes << "\n" ;
//             exit( 1 ) ;
//
//           }
//
//
//
//           // Computing angle between directions
//           float angle = atlasBundleData.computeAngleBetweenDirections(
//                                                        directionVectorFiber1,
//                                                        directionVectorFiber2 ) ;
//
//           indexAnglesAtlasBundle = atlasBundleFiberIndex_1 *
//                                                  ( 2 * nbFibersAtlasBundle - 1 -
//                                                  atlasBundleFiberIndex_1 ) / 2 +
//                                                   atlasBundleFiberIndex_2 -
//                                                    atlasBundleFiberIndex_1 - 1 ;
//
//           // if ( angle > 90 )
//           // {
//           //
//           //   angle = 180 - angle ;
//           //
//           // }
//
//           directionAnglesAtlasBundle[ indexAnglesAtlasBundle ] = angle ;
//
//         }
//
//       }
//
//     }
//
//   }
//
// }

void computeDirectionAnglesBundle(
                                BundlesData& atlasBundleData,
                                const std::vector<float>& normalVectorsBundle,
                                int nbPoints,
                                std::vector<float>& directionAnglesAtlasBundle )
{

  int nbFibersAtlasBundle = atlasBundleData.curves_count ;

  int indexAnglesAtlasBundle = 0 ;

  #pragma omp parallel for
  for ( int atlasBundleFiberIndex_1 = 0 ;
                             atlasBundleFiberIndex_1 < nbFibersAtlasBundle ;
                                                    atlasBundleFiberIndex_1++ )
  {

    std::vector<float> normalVector1( 3, 0 ) ;
    for ( int i =0 ; i < 3 ; i++ )
    {

      normalVector1[ i ] = normalVectorsBundle[
                                             3 * atlasBundleFiberIndex_1 + i ] ;

    }

    std::vector<float> fiber1( 3 * nbPoints, 0 ) ;
    atlasBundleData.getFiberFromTractogram( atlasBundleData.matrixTracks,
                                            atlasBundleFiberIndex_1,
                                            nbPoints,
                                            fiber1 ) ;

    std::vector<float> directionVectorFiber1( 3, 0 ) ;
    atlasBundleData.computeDirectionVectorFiberTractogram(
                                                       fiber1,
                                                       normalVector1,
                                                       directionVectorFiber1 ) ;


    if ( atlasBundleFiberIndex_1 + 1 < nbFibersAtlasBundle )
    {

      for ( int atlasBundleFiberIndex_2 = atlasBundleFiberIndex_1 + 1 ;
                                 atlasBundleFiberIndex_2 < nbFibersAtlasBundle ;
                                                      atlasBundleFiberIndex_2++ )
      {

        if ( atlasBundleFiberIndex_2 != atlasBundleFiberIndex_1 )
        {

          std::vector<float> normalVector2( 3, 0 ) ;
          for ( int i =0 ; i < 3 ; i++ )
          {

            normalVector2[ i ] = normalVectorsBundle[
                                             3 * atlasBundleFiberIndex_2 + i ] ;

          }

          std::vector<float> fiber2( 3 * nbPoints, 0 ) ;
          atlasBundleData.getFiberFromTractogram( atlasBundleData.matrixTracks,
                                                  atlasBundleFiberIndex_2,
                                                  nbPoints,
                                                  fiber2 ) ;

          std::vector<float> directionVectorFiber2( 3, 0 ) ;
          atlasBundleData.computeDirectionVectorFiberTractogram(
                                                       fiber2,
                                                       normalVector2,
                                                       directionVectorFiber2 ) ;



          // Computing angle between directions
          float angle = atlasBundleData.computeAngleBetweenDirections(
                                                       directionVectorFiber1,
                                                       directionVectorFiber2 ) ;

          indexAnglesAtlasBundle = atlasBundleFiberIndex_1 *
                                                 ( 2 * nbFibersAtlasBundle - 1 -
                                                 atlasBundleFiberIndex_1 ) / 2 +
                                                  atlasBundleFiberIndex_2 -
                                                   atlasBundleFiberIndex_1 - 1 ;

          // if ( angle > 90 )
          // {
          //
          //   angle = 180 - angle ;
          //
          // }

          directionAnglesAtlasBundle[ indexAnglesAtlasBundle ] = angle ;

        }

      }

    }

  }

}

//----------------------------------------------------------------------------//
//--------------------- Compute direction angles bundle ----------------------//
//----------------------------------------------------------------------------//
void computeShapeAnglesBundle(
                        BundlesData& atlasBundleData,
                        int nbPoints,
                        const std::vector<float>& medialPointsAtlasBundleFibers,
                        std::vector<float>& shapeAnglesAtlasBundle )
{

  int nbFibersAtlasBundle = atlasBundleData.curves_count ;

  #pragma omp parallel for
  for ( int atlasBundleFiberIndex = 0 ;
                             atlasBundleFiberIndex < nbFibersAtlasBundle ;
                                                      atlasBundleFiberIndex++ )
  {

    int offsetAtlas = 3 * nbPoints * atlasBundleFiberIndex ;

    std::vector<float> point1( 3, 0 ) ;
    std::vector<float> point2( 3, 0 ) ;

    std::vector<float> vector1( 3, 0 ) ;
    std::vector<float> vector2( 3, 0 ) ;
    for ( int i = 0 ; i < 3 ; i++ )
    {

      point1[ i ] = atlasBundleData[ offsetAtlas + i ] ;
      point2[ i ] = atlasBundleData[ offsetAtlas + 3 * ( nbPoints - 1 ) + i ] ;

      vector1[ i ] = point1[ i ] - medialPointsAtlasBundleFibers[
                                               3 * atlasBundleFiberIndex + i ] ;

      vector2[ i ] = point2[ i ] - medialPointsAtlasBundleFibers[
                                               3 * atlasBundleFiberIndex + i ] ;

    }

    float angle = atlasBundleData.computeAngleBetweenVectors( vector1,
                                                                     vector2 ) ;

    shapeAnglesAtlasBundle[ atlasBundleFiberIndex ] = angle ;

  }

}

//----------------------------------------------------------------------------//
//------------------- Compute min similarity measure (dMDA) ------------------//
//----------------------------------------------------------------------------//
void computeAverageDisimilarity(
                        BundlesData& atlasBundleData,
                        const std::vector<float>& normalVectorsBundle,
                        const std::vector<float>& medialPointsAtlasBundleFibers,
                        int nbPoints,
                        std::vector<float>& disimilaritiesAtlasBundle )
{

  int nbFibersAtlasBundle = atlasBundleData.curves_count ;

  int indexDisimilarityAtlasBundle = 0 ;

  #pragma omp parallel for
  for ( int atlasBundleFiberIndex_1 = 0 ;
                             atlasBundleFiberIndex_1 < nbFibersAtlasBundle ;
                                                    atlasBundleFiberIndex_1++ )
  {

    std::vector<float> normalVector1( 3 , 0 ) ;
    for ( int i = 0 ; i < 3 ; i++ )
    {

      normalVector1[ i ] = normalVectorsBundle[
                                             3 * atlasBundleFiberIndex_1 + i ] ;

    }

    std::vector<float> medialPoint1( 3, 0 ) ;
    for ( int i = 0 ; i < 3 ; i++ )
    {

      medialPoint1[ i ] = medialPointsAtlasBundleFibers[
                                             3 * atlasBundleFiberIndex_1 + i ] ;

    }

    std::vector<float> fiber1( 3 * nbPoints, 0 ) ;
    std::vector<float> newNormalVectorFiber1( 3, 0 ) ;
    atlasBundleData.registerFiberToPlaneXYAndDirectionX(
                            atlasBundleData.matrixTracks,
                            normalVector1,
                            medialPoint1,
                            atlasBundleFiberIndex_1,
                            nbPoints,
                            fiber1,
                            newNormalVectorFiber1 ) ;


    if ( atlasBundleFiberIndex_1 + 1 < nbFibersAtlasBundle )
    {

      for ( int atlasBundleFiberIndex_2 = atlasBundleFiberIndex_1 + 1 ;
                    atlasBundleFiberIndex_2 < nbFibersAtlasBundle ;
                                                     atlasBundleFiberIndex_2++ )
      {

        std::vector<float> normalVector2( 3 , 0 ) ;
        for ( int i = 0 ; i < 3 ; i++ )
        {

          normalVector2[ i ] = normalVectorsBundle[
                                           3 * atlasBundleFiberIndex_2 + i ] ;

        }

        std::vector<float> medialPoint2( 3, 0 ) ;
        for ( int i = 0 ; i < 3 ; i++ )
        {

          medialPoint2[ i ] = medialPointsAtlasBundleFibers[
                                           3 * atlasBundleFiberIndex_2 + i ] ;

        }

        std::vector<float> fiber2( 3 * nbPoints, 0 ) ;
        std::vector<float> newNormalVectorFiber2( 3, 0 ) ;
        atlasBundleData.registerFiberToPlaneXYAndDirectionX(
                                atlasBundleData.matrixTracks,
                                normalVector2,
                                medialPoint2,
                                atlasBundleFiberIndex_2,
                                nbPoints,
                                fiber2,
                                newNormalVectorFiber2 ) ;



        // Computing MDA distance
        std::vector<float> origin( 3, 0 ) ;
        float dMDA = atlasBundleData.computeMDADBetweenTwoFibers( fiber1,
                                                                  fiber2,
                                                                  origin,
                                                                  origin,
                                                                  0,
                                                                  0,
                                                                  nbPoints ) ;


        indexDisimilarityAtlasBundle = atlasBundleFiberIndex_1 *
                                               ( 2 * nbFibersAtlasBundle - 1 -
                                                atlasBundleFiberIndex_1 ) / 2 +
                                                  atlasBundleFiberIndex_2 -
                                                  atlasBundleFiberIndex_1 - 1 ;

        disimilaritiesAtlasBundle[ indexDisimilarityAtlasBundle ] = dMDA ;

      }

    }

  }

}


//----------------------------------------------------------------------------//
//------------------- Compute min similarity measure (MDF) ------------------//
//----------------------------------------------------------------------------//
void computeAverageDisimilarityMDF(
                        BundlesData& atlasBundleData,
                        const std::vector<float>& normalVectorsBundle,
                        const std::vector<float>& medialPointsAtlasBundleFibers,
                        int nbPoints,
                        std::vector<float>& disimilaritiesAtlasBundle )
{

  int nbFibersAtlasBundle = atlasBundleData.curves_count ;

  int indexDisimilarityAtlasBundle = 0 ;

  #pragma omp parallel for
  for ( int atlasBundleFiberIndex_1 = 0 ;
                             atlasBundleFiberIndex_1 < nbFibersAtlasBundle ;
                                                    atlasBundleFiberIndex_1++ )
  {

    std::vector<float> medialPoint1( 3, 0 ) ;
    for ( int i = 0 ; i < 3 ; i++ )
    {

      medialPoint1[ i ] = medialPointsAtlasBundleFibers[
                                             3 * atlasBundleFiberIndex_1 + i ] ;

    }

    if ( atlasBundleFiberIndex_1 + 1 < nbFibersAtlasBundle )
    {

      for ( int atlasBundleFiberIndex_2 = atlasBundleFiberIndex_1 + 1 ;
                    atlasBundleFiberIndex_2 < nbFibersAtlasBundle ;
                                                     atlasBundleFiberIndex_2++ )
      {

        std::vector<float> medialPoint2( 3, 0 ) ;
        for ( int i = 0 ; i < 3 ; i++ )
        {

          medialPoint2[ i ] = medialPointsAtlasBundleFibers[
                                           3 * atlasBundleFiberIndex_2 + i ] ;

        }

        // Computing MDA distance
        float dMDF = atlasBundleData.computeMDADBetweenTwoFibers(
                                                   atlasBundleData.matrixTracks,
                                                   atlasBundleData.matrixTracks,
                                                   medialPoint1,
                                                   medialPoint2,
                                                   atlasBundleFiberIndex_1,
                                                   atlasBundleFiberIndex_2,
                                                   nbPoints ) ;


        indexDisimilarityAtlasBundle = atlasBundleFiberIndex_1 *
                                               ( 2 * nbFibersAtlasBundle - 1 -
                                                atlasBundleFiberIndex_1 ) / 2 +
                                                  atlasBundleFiberIndex_2 -
                                                  atlasBundleFiberIndex_1 - 1 ;

        disimilaritiesAtlasBundle[ indexDisimilarityAtlasBundle ] = dMDF ;

      }

    }

  }

}

//----------------------------------------------------------------------------//
//---------------------------- Compute mean vector ---------------------------//
//----------------------------------------------------------------------------//
inline float computeMeanVector( const std::vector<float>& inputVector )
{

  double sum = std::accumulate( inputVector.begin(), inputVector.end(), 0.0 );
  double mean = sum / inputVector.size() ;

  return( mean ) ;

}

//----------------------------------------------------------------------------//
//--------------------- Compute standard deviation vector --------------------//
//----------------------------------------------------------------------------//
inline float computeStdVector( const std::vector<float>& inputVector )
{

  double sum = std::accumulate( inputVector.begin(), inputVector.end(), 0.0 );
  double mean = sum / inputVector.size() ;

  std::vector<double> diff( inputVector.size() );
  std::transform( inputVector.begin(), inputVector.end(), diff.begin(),
                                  std::bind2nd( std::minus<double>(), mean ) ) ;
  double sq_sum = std::inner_product( diff.begin(), diff.end(), diff.begin(),
                                                                         0.0 ) ;
  double stdev = std::sqrt( sq_sum / inputVector.size() ) ;

  return( stdev ) ;

}

//----------------------------------------------------------------------------//
//----------------------- Compute minimum value vector -----------------------//
//----------------------------------------------------------------------------//
inline float computeMinVector( const std::vector<float>& inputVector )
{

  float minValue = *( std::min_element( inputVector.begin(),
                                                         inputVector.end() ) ) ;
  return minValue ;

}

//----------------------------------------------------------------------------//
//----------------------- Compute minimum value vector -----------------------//
//----------------------------------------------------------------------------//
inline float computeMaxVector( const std::vector<float>& inputVector )
{

  float maxValue = *( std::max_element( inputVector.begin(),
                                                         inputVector.end() ) ) ;
  return maxValue ;

}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// MAIN /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main( int argc, char* argv[] )
{

  int index_atlas, index_format, index_reference, index_averageFibers,
                                       index_useMDF, index_verbose, index_help ;
  index_atlas =   getFlagPosition( argc, argv, "-a") ;
  index_format =   getFlagPosition( argc, argv, "-f") ;
  index_reference =   getFlagPosition( argc, argv, "-r") ;
  index_averageFibers = getFlagPosition( argc, argv, "-af") ;
  index_useMDF = getFlagPosition( argc, argv, "-useMDF" ) ;
  index_verbose =   getFlagPosition( argc, argv, "-v") ;
  index_help =   getFlagPosition( argc, argv, "-h") ;

  if ( index_help > 0 )
  {

    std::cout << "Function to convert bundles format : \n"
              << "-a : Directory with the atlas (one file per bundle) \n"
              << "-f : Format of atlas bundles ( optios = [ .bundles, .trk, "
              << ".tck ] ) \n"
              << "[-r] : Reference .nii image where the atlas is \n"
              << "[-af] : Output directory where to save the average fibers of "
              << "the bundles \n"
              << "[-useMDF] : Use MDF distance as disimilarity measure\n"
              << "[-v] : set verbosity level at 1 \n"
              << "[-h] : Show this message " << std::endl ;
    exit( 1 ) ;

  }

  std::string atlasDirectory ;
  if ( !index_atlas )
  {

    std::cout << "-a argument required ..." << std::endl ;
    exit( 1 ) ;

  }
  else
  {

    atlasDirectory = argv[ index_atlas + 1 ] ;
    char lastChar = atlasDirectory[ atlasDirectory.size() - 1 ] ;
    if ( lastChar != '/' )
    {

      atlasDirectory = atlasDirectory + "/" ;

    }

  }

  std::string format ;
  if ( index_format )
  {

    format = argv[ index_format + 1 ] ;

    if ( format != ".bundles" && format != ".trk" && format != ".tck" )
    {

      std::cout << "The only supported formats for the atlas bundles are "
                << ".bundles, .trk and .tck" << std::endl ;
      exit( 1 ) ;

    }

  }
  else
  {

    std::cout << "-f argument required..." << std::endl ;
    exit( 1 ) ;

  }

  std::string niiFilename ;
  if ( index_reference )
  {

    niiFilename = argv[ index_reference + 1 ] ;
    char lastChar = niiFilename[ niiFilename.size() - 1 ] ;
    if ( lastChar == '/' )
    {

      niiFilename = niiFilename.substr( 0, niiFilename.size() - 1 ) ;

    }

  }

  if ( index_useMDF )
  {

    useMDFDistance = true ;

  }

  if ( index_verbose )
  {
    if ( argv[ index_verbose + 1 ] )
    {

      verbose = std::stoi( argv[ index_verbose + 1 ] ) ;

    }
    else
    {

      verbose = 1 ;

    }

  }

  /////////////////////////////////////////////////////////////////////////////

  // Cheking format reference image
  if ( index_reference )
  {

    if ( niiFilename.find( ".nii") == std::string::npos ||
                            niiFilename.find( ".nii.gz") != std::string::npos )
    {

      std::cout << "Error format : the only supported format for the reference "
                << "image is .nii " << std::endl ;
      exit( 1 ) ;

    }

  }

  std::string averageFibersDirectory ;
  if ( index_averageFibers )
  {

    averageFibersDirectory = argv[ index_averageFibers + 1 ] ;

    char lastChar = averageFibersDirectory[ averageFibersDirectory.size()
                                                                      - 1 ] ;
    if ( lastChar != '/' )
    {

      averageFibersDirectory = averageFibersDirectory + "/" ;

    }

  }



  //   xxxxxxxxxxxxxxxxxxxxxxxxxx Reading Atlas xxxxxxxxxxxxxxxxxxxxxxxxxx   //

  std::vector< std::string > atlasBundlesFilenames ;
  std::string tmpBundlesFilename ;

  for ( const auto & file : std::experimental::filesystem::directory_iterator(
                                                    atlasDirectory.c_str() ) )
  {

    tmpBundlesFilename = file.path() ;

    if ( endswith( tmpBundlesFilename, format) )
    {
;
      atlasBundlesFilenames.push_back( tmpBundlesFilename ) ;

    }

  }

  bool isBundles = false ;
  bool isTrk = false ;
  bool isTck = false ;

  if ( format == ".bundles" )
  {

    isBundles = true ;

  }
  else if ( format == ".trk" )
  {

    isTrk = true ;

  }
  else
  {

    isTck = true ;

  }



  bool isBundlesFormat = true ;
  bool isTRKFormat = false ;
  AtlasBundles atlasData( atlasDirectory.c_str(),
                          isBundles,
                          isTrk,
                          isTck,
                          verbose ) ;



  //////////////////////////////// Sanity cheks ////////////////////////////////
  int nbPoints = atlasData.bundlesData[ 0 ].pointsPerTrack[ 0 ] ;
  int nbBundlesAtlas = atlasData.bundlesData.size() ;


  for ( int bundleIndex = 0 ; bundleIndex < nbBundlesAtlas ; bundleIndex++ )
  {

    BundlesData& bundleFibers = atlasData.bundlesData[ bundleIndex ] ;

    int nbFibersBundle = bundleFibers.curves_count ;

    for ( int fiber = 1 ; fiber < nbFibersBundle ; fiber++ )
    {

      if ( bundleFibers.pointsPerTrack[ fiber ] != nbPoints )
      {

        std::cout << "Error atlas : the number of points in each fiber "
                  << "of the atlas has to be the same, got "
                  << bundleFibers.pointsPerTrack[ fiber ]
                  << " and " << nbPoints << " for fiber " << fiber
                  << " in bundle " << bundleIndex << " and for fiber 0 in "
                  << "bundle 0" << std::endl ;
        exit( 1 ) ;

      }

    }

  }



  ////////////////////////////// Analysing Atlas ///////////////////////////////

  // Looping over bundles
  for ( int atlasBundleIndex = 0 ; atlasBundleIndex < nbBundlesAtlas ;
                                                      atlasBundleIndex++ )
  {

    BundlesMinf& atlasBundleInfo = atlasData.bundlesMinf[ atlasBundleIndex ] ;
    BundlesData& atlasBundleData = atlasData.bundlesData[
                                                            atlasBundleIndex ] ;

    if ( verbose )
    {

      printf( "\rProcessing atlas bundles : [ %d  /  %d ]",
                                        atlasBundleIndex + 1, nbBundlesAtlas ) ;
      fflush( stdout ) ;

    }

    int nbFibersAtlasBundle = atlasBundleData.curves_count ;

    // Compute center points of atlas bundle fibers
    std::vector<float> medialPointsAtlasBundleFibers(
                                                  3 * nbFibersAtlasBundle, 0 ) ;

    computeCenterAtlasBundleFibers( atlasBundleData,
                                    medialPointsAtlasBundleFibers ) ;

    // Compute average atlas bundle fiber
    std::vector<float> averageFiber( 3 * nbPoints, 0 ) ;
    std::vector<float> medialPointAtlasBundle( 3, 0 ) ;
    computeAverageFiberBundle( atlasBundleData,
                               medialPointsAtlasBundleFibers,
                               nbPoints,
                               averageFiber,
                               medialPointAtlasBundle ) ;

    std::vector<float> gravityCenterAtlasBundle( 3, 0 ) ;
    computeGravityCenterAtlasBundle( atlasBundleData,
                                     nbPoints,
                                     gravityCenterAtlasBundle ) ;

    atlasBundleInfo.centerBundle[ 0 ] = gravityCenterAtlasBundle[ 0 ] ;
    atlasBundleInfo.centerBundle[ 1 ] = gravityCenterAtlasBundle[ 1 ] ;
    atlasBundleInfo.centerBundle[ 2 ] = gravityCenterAtlasBundle[ 2 ] ;

    if ( index_averageFibers )
    {

      std::string bundleName = getFilenameNoExtension(
                                  atlasBundlesFilenames[ atlasBundleIndex ] ) ;

      std::string averageFiberBundlesFilename = averageFibersDirectory +
                                         "average_" + bundleName + ".bundles" ;

      std::string averageFiberBundlesDataFilename = averageFibersDirectory +
                                     "average_" + bundleName + ".bundlesdata" ;

      BundlesMinf averageFiberBundlesInfo( atlasBundleInfo ) ;
      averageFiberBundlesInfo.curves_count = 1 ;

      BundlesData averageFiberBundlesData ;
      averageFiberBundlesData.curves_count = 1 ;

      averageFiberBundlesData.pointsPerTrack.push_back( nbPoints ) ;

      averageFiberBundlesData.matrixTracks = averageFiber ;

      averageFiberBundlesInfo.write( averageFiberBundlesFilename.c_str() ) ;

      averageFiberBundlesData.write( averageFiberBundlesDataFilename.c_str(),
                                     averageFiberBundlesInfo ) ;

    }


    // Compute radius atlas bundle
    std::vector<float> distancesToCenterAtlasBundle( nbFibersAtlasBundle, 0 ) ;
    computeDistancesToCenterBundle( medialPointsAtlasBundleFibers,
                                    gravityCenterAtlasBundle,
                                    nbFibersAtlasBundle,
                                    distancesToCenterAtlasBundle ) ;

    // Since accumulate is used in vector of floats, it is necesary to put 0.0f
    // instead of 0 or to not put anything at all at the end
    // float averageRadius = computeMeanVector( distancesToCenterAtlasBundle ) ;
    // float stdRadius = computeStdVector( distancesToCenterAtlasBundle ) ;
    // atlasBundleInfo.averageRadius = averageRadius ;
    // if ( averageRadius - 3 * stdRadius > 0 )
    // {
    //
    //   atlasBundleInfo.minRadius = averageRadius - 3 * stdRadius ;
    //
    // }
    // else
    // {
    //
    //   atlasBundleInfo.minRadius = 0 ;
    //
    // }
    // atlasBundleInfo.maxRadius = averageRadius + 3 * stdRadius ;

    float averageRadius = computeMeanVector( distancesToCenterAtlasBundle ) ;
    atlasBundleInfo.averageRadius = averageRadius ;
    float minRadius = computeMinVector( distancesToCenterAtlasBundle ) ;
    atlasBundleInfo.minRadius = minRadius ;
    float maxRadius = computeMaxVector( distancesToCenterAtlasBundle ) ;
    atlasBundleInfo.maxRadius = maxRadius ;



    // Compute angle atlas bundle
    std::vector<float> normalVectorsBundle( 3 * nbFibersAtlasBundle, 0 ) ;
    computeNormalVectorFibersAtlasBundle( atlasBundleData,
                                          normalVectorsBundle ) ;

    int32_t numberAnglesAtlasBundle = ( nbFibersAtlasBundle - 1 ) *
                                              ( nbFibersAtlasBundle ) / 2 ;
    std::vector<float> anglesAtlasBundle( numberAnglesAtlasBundle, 0 ) ;
    computeAnglesBundle( atlasBundleData,
                         normalVectorsBundle,
                         anglesAtlasBundle ) ;

    // float averageAngle = computeMeanVector( anglesAtlasBundle ) ;
    // float stdAngle = computeStdVector( anglesAtlasBundle ) ;
    // atlasBundleInfo.averageAngle = averageAngle ;
    // if ( averageAngle - 3 * stdAngle > 0 )
    // {
    //
    //   atlasBundleInfo.minAngle = averageAngle - 3 * stdAngle ;
    //
    // }
    // else
    // {
    //
    //   atlasBundleInfo.minAngle = 0 ;
    //
    // }
    // atlasBundleInfo.maxAngle = averageAngle + 3 * stdAngle ;

    float averageAngle = computeMeanVector( anglesAtlasBundle );
    atlasBundleInfo.averageAngle = averageAngle ;
    float minAngle = computeMinVector( anglesAtlasBundle ) ;
    atlasBundleInfo.minAngle = minAngle ;
    float maxAngle = computeMaxVector( anglesAtlasBundle ) ;
    atlasBundleInfo.maxAngle = maxAngle ;



    // Compute direction angles atlas bundles
    std::vector<float> directionAnglesAtlasBundle( numberAnglesAtlasBundle,
                                                                           0 ) ;
    computeDirectionAnglesBundle( atlasBundleData,
                                  normalVectorsBundle,
                                  nbPoints,
                                  directionAnglesAtlasBundle ) ;

    // float averageDirectionAngle = computeMeanVector(
    //                                               directionAnglesAtlasBundle ) ;
    // float stdDirectionAngle = computeStdVector( directionAnglesAtlasBundle ) ;
    // atlasBundleInfo.averageDirectionAngle = averageDirectionAngle ;
    // if ( averageDirectionAngle - 3 * stdDirectionAngle > 0 )
    // {
    //
    //   atlasBundleInfo.minDirectionAngle = averageDirectionAngle -
    //                                                      3 * stdDirectionAngle ;
    //
    // }
    // else
    // {
    //
    //   atlasBundleInfo.minDirectionAngle = 0 ;
    //
    // }
    // atlasBundleInfo.maxDirectionAngle = averageDirectionAngle +
    //                                                      3 * stdDirectionAngle ;

    float averageDirectionAngle = computeMeanVector(
                                                  directionAnglesAtlasBundle ) ;
    atlasBundleInfo.averageDirectionAngle = averageDirectionAngle ;
    float minDirectionAngle = computeMinVector( directionAnglesAtlasBundle ) ;
    atlasBundleInfo.minDirectionAngle = minDirectionAngle ;
    float maxDirectionAngle = computeMaxVector( directionAnglesAtlasBundle ) ;
    atlasBundleInfo.maxDirectionAngle = maxDirectionAngle ;



    // Compute the angle between the vectors formed by central point - first
    // point and central point - last points
    std::vector<float> shapeAnglesAtlasBundle( nbFibersAtlasBundle, 0 ) ;
    computeShapeAnglesBundle( atlasBundleData,
                              nbPoints,
                              medialPointsAtlasBundleFibers,
                              shapeAnglesAtlasBundle ) ;

    // float averageShapeAngle = computeMeanVector( shapeAnglesAtlasBundle ) ;
    // float stdShapeAngle = computeStdVector( shapeAnglesAtlasBundle ) ;
    // atlasBundleInfo.averageShapeAngle = averageShapeAngle ;
    // if ( averageShapeAngle - 3 * stdShapeAngle > 0 )
    // {
    //
    //   atlasBundleInfo.minShapeAngle = averageShapeAngle - 3 * stdShapeAngle ;
    //
    // }
    // else
    // {
    //
    //   atlasBundleInfo.minShapeAngle = 0 ;
    //
    // }
    // atlasBundleInfo.maxShapeAngle = averageShapeAngle + 3 * stdShapeAngle  ;


    float averageShapeAngle = computeMeanVector( shapeAnglesAtlasBundle ) ;
    atlasBundleInfo.averageShapeAngle = averageShapeAngle ;
    float minShapeAngle = computeMinVector( shapeAnglesAtlasBundle ) ;
    atlasBundleInfo.minShapeAngle = minShapeAngle ;
    float maxShapeAngle = computeMaxVector( shapeAnglesAtlasBundle ) ;
    atlasBundleInfo.maxShapeAngle = maxShapeAngle ;



    // Compute lenght fibers atlas bundle
    std::vector<float> lengthsAtlasBundle( nbFibersAtlasBundle, 0 ) ;
    atlasData.computeLengthsAtlasBundleFibers( atlasBundleData,
                                               nbPoints,
                                               lengthsAtlasBundle ) ;

    // float averageLength = computeMeanVector( lengthsAtlasBundle ) ;
    // float stdLength = computeStdVector( lengthsAtlasBundle ) ;
    // atlasBundleInfo.averageLength = averageLength ;
    // if ( averageLength - 3 * stdLength > 0 )
    // {
    //
    //   atlasBundleInfo.minLength = averageLength - 3 * stdLength ;
    //
    // }
    // else
    // {
    //
    //   atlasBundleInfo.minLength = 0 ;
    //
    // }
    // atlasBundleInfo.maxLength = averageLength + 3 * stdLength ;


    float averageLength = computeMeanVector( lengthsAtlasBundle ) ;
    atlasBundleInfo.averageLength = averageLength ;
    float minLength = computeMinVector( lengthsAtlasBundle ) ;
    atlasBundleInfo.minLength = minLength ;
    float maxLength = computeMaxVector( lengthsAtlasBundle ) ;
    atlasBundleInfo.maxLength = maxLength ;



    // Compute disimilarity atlas bundle
    int32_t numberDisimilaritiesAtlasBundle = ( nbFibersAtlasBundle - 1) *
                                              ( nbFibersAtlasBundle ) / 2 ;

    std::vector<float> disimilaritiesAtlasBundle(
                                          numberDisimilaritiesAtlasBundle, 0 ) ;
    if ( !useMDFDistance )
    {

      computeAverageDisimilarity( atlasBundleData,
                                  normalVectorsBundle,
                                  medialPointsAtlasBundleFibers,
                                  nbPoints,
                                  disimilaritiesAtlasBundle ) ;

    }
    else
    {

      computeAverageDisimilarityMDF( atlasBundleData,
                                     normalVectorsBundle,
                                     medialPointsAtlasBundleFibers,
                                     nbPoints,
                                     disimilaritiesAtlasBundle ) ;

    }


    // float averageDisimilarity = computeMeanVector( disimilaritiesAtlasBundle ) ;
    // float stdDisimilarity = computeStdVector( disimilaritiesAtlasBundle ) ;
    // atlasBundleInfo.averageDisimilarity = averageDisimilarity ;
    // if ( averageDisimilarity - 3 * stdDisimilarity > 0 )
    // {
    //
    //   atlasBundleInfo.minDisimilarity = averageDisimilarity -
    //                                                        3 * stdDisimilarity ;
    //
    // }
    // else
    // {
    //
    //   atlasBundleInfo.minDisimilarity = 0 ;
    //
    // }
    // atlasBundleInfo.maxDisimilarity = averageDisimilarity +
    //                                                        3 * stdDisimilarity ;

    //
    float averageDisimilarity = computeMeanVector( disimilaritiesAtlasBundle ) ;
    atlasBundleInfo.averageDisimilarity = averageDisimilarity ;
    float minDisimilarity = computeMinVector( disimilaritiesAtlasBundle ) ;
    atlasBundleInfo.minDisimilarity = minDisimilarity ;
    float maxDisimilarity = computeMaxVector( disimilaritiesAtlasBundle ) ;
    atlasBundleInfo.maxDisimilarity = maxDisimilarity ;




    // Computing distance between medial points
    int32_t numberDistancesBetweenMedialPoints = ( nbFibersAtlasBundle - 1) *
                                                   ( nbFibersAtlasBundle ) / 2 ;
    std::vector<float> distancesBetweenMedialPointsBundle(
                                       numberDistancesBetweenMedialPoints, 0 ) ;
    computeDistancesBetweenMedialPointsBundle(
                                          atlasBundleData,
                                          medialPointsAtlasBundleFibers,
                                          distancesBetweenMedialPointsBundle ) ;


    // float averageDistanceBetweenMedialPoints = computeMeanVector(
    //                                       distancesBetweenMedialPointsBundle ) ;
    // float stdDistanceBetweenMedialPoints = computeStdVector(
    //                                       distancesBetweenMedialPointsBundle ) ;
    // atlasBundleInfo.averageDistanceBetweenMedialPoints =
    //                                         averageDistanceBetweenMedialPoints ;
    // if ( averageDistanceBetweenMedialPoints -
    //                                     3 * stdDistanceBetweenMedialPoints > 0 )
    // {
    //
    //   atlasBundleInfo.minDistanceBetweenMedialPoints =
    //                                         averageDistanceBetweenMedialPoints -
    //                                         3 * stdDistanceBetweenMedialPoints ;
    //
    // }
    // else
    // {
    //
    //   atlasBundleInfo.minDistanceBetweenMedialPoints = 0 ;
    //
    // }
    // atlasBundleInfo.maxDistanceBetweenMedialPoints =
    //                                         averageDistanceBetweenMedialPoints +
    //                                         3 * stdDistanceBetweenMedialPoints ;

    float averageDistanceBetweenMedialPoints = computeMeanVector(
                                          distancesBetweenMedialPointsBundle ) ;
    atlasBundleInfo.averageDistanceBetweenMedialPoints =
                                            averageDistanceBetweenMedialPoints ;
    float minDistanceBetweenMedialPoints = computeMinVector(
                                          distancesBetweenMedialPointsBundle ) ;
    atlasBundleInfo.minDistanceBetweenMedialPoints =
                                                minDistanceBetweenMedialPoints ;
    float maxDistanceBetweenMedialPoints = computeMaxVector(
                                          distancesBetweenMedialPointsBundle ) ;
    atlasBundleInfo.maxDistanceBetweenMedialPoints =
                                                maxDistanceBetweenMedialPoints ;




    // Computing density
    float density = atlasBundleInfo.curves_count / (
                          4 / 3 * M_PI * pow( atlasBundleInfo.maxRadius, 3 ) ) ;
    atlasBundleInfo.density = density ;



  }


  /////////////////////////////// Saving results ///////////////////////////////
  if ( verbose )
  {

    std::cout << "\n" ;

  }


  if ( verbose )
  {

    std::cout << "Checking other information in .bundles " << std::endl ;

  }

  // Checking other information in .bundles files
  if ( index_reference )
  {

    NiftiImage niiHeader( niiFilename.c_str() ) ;
    for ( int bundle = 0 ; bundle < nbBundlesAtlas ; bundle++ )
    {

      for ( int i = 0 ; i < 3 ; i++ )
      {

        if ( atlasData.bundlesMinf[ bundle ].resolution[ i ] == 0 )
        {

          atlasData.bundlesMinf[ bundle ].resolution[ i ] =
                                                     niiHeader.resolution[ i ] ;

        }

        if ( atlasData.bundlesMinf[ bundle ].size[ i ] == 0 )
        {

          atlasData.bundlesMinf[ bundle ].size[ i ] = niiHeader.size[ i ] ;

        }


      }

      if ( atlasData.bundlesMinf[ bundle ].space_dimension == 0 )
      {

        atlasData.bundlesMinf[ bundle ].space_dimension =  3 ;

      }

    }

  }

  // Saving .bundles files

  if ( verbose )
  {

    std::cout << "Saving .bundles " << std::endl ;

  }

  for ( int bundle = 0 ; bundle < nbBundlesAtlas ; bundle++ )
  {

    atlasData.bundlesMinf[ bundle ].write(
                                     atlasBundlesFilenames[ bundle ].c_str() ) ;

  }

  if ( verbose )
  {

    std::cout << "Done" << std::endl ;

  }

}
