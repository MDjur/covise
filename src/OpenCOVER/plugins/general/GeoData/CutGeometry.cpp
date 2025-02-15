/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include "CutGeometry.h"

#include <cover/coVRFileManager.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>
#include <osgDB/Options>
#include <osgDB/ReadFile>
#include <osg/PositionAttitudeTransform>
#include <osg/LightModel>


#include <osgDB/WriteFile>

#include <osg/Depth>
#include "GeoDataLoader.h"

using namespace opencover;

//CutGeometry::CutGeometry(osg::Vec3d offset_, const std::vector<OGRLayer*>& layers_)
CutGeometry::CutGeometry(GeoDataLoader *plugin_)
    : plugin(plugin_)
{
    //std::vector<std::string> treeFileVector;
    //treeFileVector.push_back("Maple_silver_alone_16_1/Maple_silver_alone_16_1.osg");
    //treeFileVector.push_back("Maple_silver_alone_21_1/Maple_silver_alone_21_1.osg");
    //treeFileVector.push_back("Maple_silver_alone_32_1/Maple_silver_alone_32_1.osg");
    /*treeFileVector.push_back("trees/tree1");
   treeFileVector.push_back("trees/tree2");
   treeFileVector.push_back("trees/tree3");
   for(int treeIt = 0; treeIt<treeFileVector.size(); ++treeIt) {
   
      osg::Node* treeNode = coVRFileManager::instance()->loadIcon(treeFileVector[treeIt].c_str());
      if(treeNode) {
         treeNode->setName("Tree");
         treeNodeVector.push_back(treeNode);
      }
   }*/

    //Define voids by aligned bounding area:
    //voidBoundingAreaVector.push_back(BoundingArea(osg::Vec2(3450410.0,5389240.0),osg::Vec2(3450510.0,5389340.0)));

    treeStateSet = new osg::StateSet;
    const char *treeFileName = coVRFileManager::instance()->getName("share/covise/materials/trees_map.png");
    if (treeFileName)
    {
        osg::Image *treeTexImage = osgDB::readImageFile(treeFileName);
        osg::Texture2D *treeTex = new osg::Texture2D;
        treeTex->ref();
        treeTex->setWrap(osg::Texture2D::WRAP_S, osg::Texture::CLAMP);
        treeTex->setWrap(osg::Texture2D::WRAP_T, osg::Texture::CLAMP);
        if (treeTexImage)
        {
            treeTex->setImage(treeTexImage);
        }
        treeStateSet->setTextureAttributeAndModes(0, treeTex);
    }

    /*osg::LightModel* lightmodel = new osg::LightModel();
   lightmodel->setTwoSided(true);
   treeStateSet->setAttributeAndModes(lightmodel);*/

    treeShader = coVRShaderList::instance()->get("treeAlpha");
    if (!treeShader)
    {
        std::cout << "------------------------ No tree shader! -------------------------" << std::endl;
    }
    else
    {
        std::cout << "------------------------ Tree shader! ----------------------------" << std::endl;

        treeShader->apply(treeStateSet.get());
    }
    /*treeStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
   treeStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
   treeStateSet->setMode( GL_DEPTH_TEST, osg::StateAttribute::ON );
   osg::Depth* treeDepth = new osg::Depth;
   treeDepth->setWriteMask( false );
   treeStateSet->setAttributeAndModes( treeDepth, osg::StateAttribute::ON );
   treeStateSet->setMode( GL_LIGHTING, osg::StateAttribute::OFF );*/

    buildingStateSet = new osg::StateSet;
    //const char *buildingFileName = coVRFileManager::instance()->getName("share/covise/materials/house_two-sides.png");
    const char *buildingFileName = coVRFileManager::instance()->getName("share/covise/materials/house-texture.jpg");
    if (buildingFileName)
    {
        osg::Image *buildingTexImage = osgDB::readImageFile(buildingFileName);
        osg::Texture2D *buildingTex = new osg::Texture2D;
        buildingTex->ref();
        buildingTex->setWrap(osg::Texture2D::WRAP_S, osg::Texture::CLAMP);
        buildingTex->setWrap(osg::Texture2D::WRAP_T, osg::Texture::CLAMP);
        if (buildingTexImage)
        {
            buildingTex->setImage(buildingTexImage);
        }
        buildingStateSet->setTextureAttributeAndModes(0, buildingTex);
    }
}

void CutGeometry::loaded(osgTerrain::TerrainTile *tile, const osgDB::ReaderWriter::Options *options) const
{
    //Change TerrainTechnique
   // tile->setTerrainTechnique(new DecoratedGeometryTechnique(voidBoundingAreaVector, treeStateSet.get(), buildingStateSet.get(), shapeFileVector));
    std::stringstream nameStream;
    nameStream << "tile_L" << tile->getTileID().level << "_X" << tile->getTileID().x << "_Y" << tile->getTileID().y;
    tile->setName(nameStream.str());

    /*tile->setName("TerrainTile");

   osg::Vec3d model;
   tile->getLocator()->convertLocalToModel(osg::Vec3d(0.0,0.0,0.0), model);
   char name[1000];
   sprintf(name,"X%f_Y%f_Z%f", model.x(), model.y(), model.z());
   tile->setName(name);*/

    //node->setNodeMask(node->getNodeMask() & (~Isect::Visible));
    osgTerrain::HeightFieldLayer *hflayer = dynamic_cast<osgTerrain::HeightFieldLayer *>(tile->getElevationLayer());
    osg::HeightField *field = NULL;
    if (hflayer)
    {
        field = hflayer->getHeightField();
    }
    else
    {
        return;
    }

    double tileXInterval = field->getXInterval();
    double tileYInterval = field->getYInterval();
    double tileElementArea = tileXInterval * tileYInterval;
    double tileXMin = plugin->offset.x() + field->getOrigin().x();
    double tileXMax = tileXMin + tileXInterval * (double)field->getNumColumns();
    double tileYMin = plugin->offset.y() + field->getOrigin().y();
    double tileYMax = tileYMin + tileYInterval * (double)field->getNumRows();
    //osg::BoundingBox tileBB(tileXMin, tileYMin, -10.000, tileXMax, tileYMax, 10.000);

    if (tileXInterval > 10.0 && tileYInterval > 10.0)
    {
        return;
    }


#if 0
   
   osg::PositionAttitudeTransform* treeGeodeTransform = new osg::PositionAttitudeTransform;
   tile->addChild(treeGeodeTransform);
   treeGeodeTransform->setName("TreeGeodeTransform");
   osg::Vec3 tileMin(tileXMin, tileYMin, 0.0);
   treeGeodeTransform->setPosition(-offset + tileMin);
   osg::Geode* treeGeode = new osg::Geode();
   //tile->addChild(treeGeode);
   treeGeodeTransform->addChild(treeGeode);
   treeGeode->setName("TreeGeode");
   treeGeode->setStateSet(treeStateSet);

#endif

#if 0
   for(int layerIt = 0; layerIt < layers.size(); ++layerIt) {
      OGRLayer* layer = layers[layerIt];
      layer->ResetReading();

      OGRFeature* poFeature = NULL;
      while((poFeature = layer->GetNextFeature()) != NULL) {
         OGRGeometry *poGeometry = poFeature->GetGeometryRef();
         if(poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon)
         {
            OGRPolygon *poPolygon = dynamic_cast<OGRPolygon*>(poGeometry);
            //std::cout << "Geometry name: " << poPolygon->getGeometryName() << std::endl;
            //std::cout << "Polgon area: " << poPolygon->get_Area() << std::endl;

            OGRSpatialReference *poSource = layer->GetSpatialRef();
            OGRSpatialReference *poTarget = new OGRSpatialReference();
            poTarget->importFromProj4("+proj=utm +zone=32");
            OGRCoordinateTransformation* poCoordTrans = OGRCreateCoordinateTransformation(poSource, poTarget);

            poPolygon->transform(poCoordTrans);
            OGRLinearRing* poRing = poPolygon->getExteriorRing();
            /*for(int pointIt = 0; pointIt<poRing->getNumPoints(); ++pointIt) {
              std::cout << "Point " << pointIt << ": (" << poRing->getX(pointIt) + offset.x()
              << ", " << poRing->getY(pointIt) + offset.y()
              << ", " << poRing->getZ(pointIt) << ")" << std::endl;
              }*/
            /*double interval = 10.0*sqrt(tileXInterval*tileXInterval + tileYInterval*tileYInterval);
            OGRPoint point;
            poRing->Value(0, &point);
            OGRPoint point_next;
            for(double s = 0; s<poRing->get_Length(); s += interval*mt()) {
               double s_next = s+interval; if(s_next>poRing->get_Length()) s_next = poRing->get_Length();
               poRing->Value(s_next, &point_next);

               double ccw = poRing->isClockwise() ? -1.0 : 1.0;

               double t_x = point_next.getX() - point.getX();
               double t_y = point_next.getY() - point.getY();
               double n_x = -t_y*ccw;
               double n_y = t_x*ccw;
               double mag_n = sqrt(n_x*n_x+n_y*n_y);
               if(mag_n>1e-3) {
                  n_x /= mag_n; n_y /= mag_n;
               }
               
               double n_mt = mt();
               double x = point.getX()+offset.x() + 0.5*t_x + 10.0*n_mt*n_x;
               double y = point.getY()+offset.y() + 0.5*t_y + 10.0*n_mt*n_y;
               //std::cout << "t_x: " << t_x << ", t_y: " << t_y << ", n_x: " << 10.0*n_mt*n_x << ", n_y: " << 10.0*n_mt*n_y << std::endl;
               std::cout << "x: " << x << ", y: " << y << std::endl;

               point = point_next;
               //std::cout << "Boundary trace: s: " << s << ", x: " << x << ", y:"  << y << ", tile min: (" << tileXMin << ", " << tileYMin << "), max: (" << tileXMax << ", " << tileYMax << ")" << std::endl;

               osg::BoundingBox tileBB(tileXMin, tileYMin, -1.0, tileXMax, tileYMax, 1.0);
               if(tileBB.contains(osg::Vec3(x,y,0.0))) {
                  osg::PositionAttitudeTransform* treeTrans = new osg::PositionAttitudeTransform();
                  treeGroup->addChild(treeTrans);
                  //hflayer->getInterpolatedValue(x,y,z);
                  int x_int = floor((x-tileXMin)/tileXInterval+0.5);
                  if(x_int<0) x_int=0; else if(x_int>=field->getNumColumns()) x_int=field->getNumColumns()-1;
                  int y_int = floor((y-tileYMin)/tileYInterval+0.5);
                  if(y_int<0) y_int=0; else if(y_int>=field->getNumRows()) y_int=field->getNumRows()-1;
                  double z = field->getHeight(x_int,y_int);

                  //osg::Vec3 treePos(x,y,z);
                  osg::Vec3 treePos(point.getX(),point.getY(),z);
                  treeTrans->setPosition(treePos);
                  int treeIt = (int)floor(mt()*(double)treeNodeVector.size());
                  treeTrans->addChild(treeNodeVector[treeIt].get());
                  treeTrans->setName("TreeTransform");
               }
            }*/


            OGREnvelope psEnvelope;
            poPolygon->getEnvelope(&psEnvelope);
            int nTrees = (int)((psEnvelope.MaxX-psEnvelope.MinX)*(psEnvelope.MaxY-psEnvelope.MinY)*1e-5/tileElementArea);
            for(int i = 0; i<nTrees; ++i)
            {
               OGRPoint point;
               //point.setX(mt()*(psEnvelope.MaxX-psEnvelope.MinX) + psEnvelope.MinX);
               //point.setY(mt()*(psEnvelope.MaxY-psEnvelope.MinY) + psEnvelope.MinY);
               point.setX(mt()*(tileXMax-tileXMin) + tileXMin);
               point.setY(mt()*(tileYMax-tileYMin) + tileYMin);
               //if(poPolygon->Contains(&point)) {
                  osg::PositionAttitudeTransform* treeTrans = new osg::PositionAttitudeTransform();
                  treeGroup->addChild(treeTrans);
                  //hflayer->getInterpolatedValue(x,y,z);
                  double x = point.getX() + offset.x();
                  double y = point.getY() + offset.y();
                  int x_int = floor((x-tileXMin)/tileXInterval+0.5);
                  if(x_int<0) x_int=0; else if(x_int>=field->getNumColumns()) x_int=field->getNumColumns()-1;
                  int y_int = floor((y-tileYMin)/tileYInterval+0.5);
                  if(y_int<0) y_int=0; else if(y_int>=field->getNumRows()) y_int=field->getNumRows()-1;
                  double z = field->getHeight(x_int,y_int);
                  std::cout << "int x: " << x_int << ", x: " << x-tileXMin << ", y: " << y_int << ", y: " << y-tileYMin << std::endl;

                  //osg::Vec3 treePos(x,y,z);
                  osg::Vec3 treePos(x,y,z);
                  treeTrans->setPosition(treePos);
                  int treeIt = (int)floor(mt()*(double)treeNodeVector.size());
                  treeTrans->addChild(treeNodeVector[treeIt].get());
                  treeTrans->setName("TreeTransform");
               //}
            }
         }
      }
   }
#endif

#if 0
   typedef gaalet::algebra< gaalet::signature<3,0> > em;
   typedef em::mv<1, 2, 4>::type Vector;
   typedef em::mv<3, 5, 6>::type Bivector;
   typedef em::mv<0, 3, 5, 6>::type Rotor;

   static const em::mv<0>::type one(1.0);
   static const em::mv<1>::type e1(1.0);
   static const em::mv<2>::type e2(1.0);
   static const em::mv<4>::type e3(1.0);
   static const em::mv<7>::type e123(1.0);

   osgTerrain::Layer* colorLayer = tile->getColorLayer(0);
   if(colorLayer) {
      osg::Image* inputImage = colorLayer->getImage();

      /*static int tileLoadCounter = 0;
      osg::Image* outputImage = NULL;
      if(tileLoadCounter==0) {
         outputImage = new osg::Image();
         outputImage->allocateImage(inputImage->s(), inputImage->t(), 1, 0x1907, 0x1401);
      }
      tileLoadCounter += 1;*/


      em::mv<0,6>::type *fftwImag1 = (em::mv<0,6>::type*) fftw_malloc(sizeof(em::mv<0,6>::type)*inputImage->s()*inputImage->t());
      em::mv<0,3>::type *fftwImag2 = (em::mv<0,3>::type*) fftw_malloc(sizeof(em::mv<0,3>::type)*inputImage->s()*inputImage->t());

      em::mv<0,6>::type *fftwFreq1 = (em::mv<0,6>::type*) fftw_malloc(sizeof(em::mv<0,6>::type)*inputImage->s()*inputImage->t());
      em::mv<0,3>::type *fftwFreq2 = (em::mv<0,3>::type*) fftw_malloc(sizeof(em::mv<0,3>::type)*inputImage->s()*inputImage->t());


      fftw_plan fftwForward1 = fftw_plan_dft_2d(inputImage->s(), inputImage->t(), reinterpret_cast<fftw_complex*>(fftwImag1), reinterpret_cast<fftw_complex*>(fftwFreq1), FFTW_FORWARD, FFTW_ESTIMATE);
      fftw_plan fftwForward2 = fftw_plan_dft_2d(inputImage->s(), inputImage->t(), reinterpret_cast<fftw_complex*>(fftwImag2), reinterpret_cast<fftw_complex*>(fftwFreq2), FFTW_FORWARD, FFTW_ESTIMATE);

      fftw_plan fftwBackward1 = fftw_plan_dft_2d(inputImage->s(), inputImage->t(), reinterpret_cast<fftw_complex*>(fftwFreq1), reinterpret_cast<fftw_complex*>(fftwImag1), FFTW_BACKWARD, FFTW_ESTIMATE);
      fftw_plan fftwBackward2 = fftw_plan_dft_2d(inputImage->s(), inputImage->t(), reinterpret_cast<fftw_complex*>(fftwFreq2), reinterpret_cast<fftw_complex*>(fftwImag2), FFTW_BACKWARD, FFTW_ESTIMATE);


      for(int x=0; x<inputImage->s(); ++x) {
         for(int y=0; y<inputImage->t(); ++y) {
            if(inputImage->getPixelFormat()==0x83f0) {   //COMPRESSED_RGB_S3TC_DXT1_EXT decompression
               unsigned int blocksize = 8;

               unsigned char* data = inputImage->data();
               unsigned char* block = data + (blocksize*(unsigned int)(ceil(double(inputImage->s()/4.0)*floor(y/4.0) + floor(x/4.0))));

               int rel_x = x % 4;
               int rel_y = y % 4;

               unsigned char& c0_lo = *(block+0);
               unsigned char& c0_hi = *(block+1);
               unsigned char& c1_lo = *(block+2);
               unsigned char& c1_hi = *(block+3);
               unsigned char& bits_0 = *(block+4);
               unsigned char& bits_1 = *(block+5);
               unsigned char& bits_2 = *(block+6);
               unsigned char& bits_3 = *(block+7);

               unsigned short color0 = c0_lo + c0_hi * 256;
               unsigned short color1 = c1_lo + c1_hi * 256;
               unsigned int bits = bits_0 + 256 * (bits_1 + 256 * (bits_2 + 256 * bits_3));

               unsigned int code_bits_pos = 2*(4*rel_y+rel_x);
               unsigned int code = (bits>>code_bits_pos) & 0x03;

               unsigned char r0 = ((color0>>11) & 0x1f);
               unsigned char g0 = ((color0>>5) & 0x3f);
               unsigned char b0 = ((color0>>0) & 0x1f);
               unsigned char r1 = ((color1>>11) & 0x1f);
               unsigned char g1 = ((color1>>5) & 0x3f);
               unsigned char b1 = ((color1>>0) & 0x1f);

               unsigned char r = 0;
               unsigned char g = 0;
               unsigned char b = 0;

               if(color0>color1) {
                  switch(code) {
                     case 0:
                        r = r0;
                        g = g0;
                        b = b0;
                        break;
                     case 1:
                        r = r1;
                        g = g1;
                        b = b1;
                        break;
                     case 2:
                        r = (2*r0+r1)/3;
                        g = (2*g0+g1)/3;
                        b = (2*b0+b1)/3;
                        break;
                     case 3:
                        r = (r0+2*r1)/3;
                        g = (g0+2*g1)/3;
                        b = (b0+2*b1)/3;
                        break;
                  };
               }
               else {
                  switch(code) {
                     case 0:
                        r = r0;
                        g = g0;
                        b = b0;
                        break;
                     case 1:
                        r = r1;
                        g = g1;
                        b = b1;
                        break;
                     case 2:
                        r = (r0+r1)/2;
                        g = (g0+g1)/2;
                        b = (b0+b1)/2;
                        break;
                     case 3:
                        r = 0;
                        g = 0;
                        b = 0;
                        break;
                  };
               }

               (*(fftwImag1+inputImage->t()*x+y)) = (0.0*one)+((double)r*(double)0xff/(double)0x1f*e2*e3);
               (*(fftwImag2+inputImage->t()*x+y)) = ((double)g*(double)0xff/(double)0x3f*e3*e1)*e3*e1 + ((double)b*(double)0xff/(double)0x1f*e1*e2);

               /*if(outputImage) {
                  *(outputImage->data(x,y)+0) = (unsigned int)((double)r*(double)0xff/(double)0x1f);
                  *(outputImage->data(x,y)+1) = (unsigned int)((double)g*(double)0xff/(double)0x3f);
                  *(outputImage->data(x,y)+2) = (unsigned int)((double)b*(double)0xff/(double)0x1f);
               }*/
            }
            else {
               unsigned char& r = *(inputImage->data(x,y)+0);
               unsigned char& g = *(inputImage->data(x,y)+1);
               unsigned char& b = *(inputImage->data(x,y)+2);

               (*(fftwImag1+inputImage->t()*x+y)) = (0.0*one)+((double)r*e2*e3);
               (*(fftwImag2+inputImage->t()*x+y)) = ((double)g*e3*e1)*e3*e1 + ((double)b*e1*e2);
               //if(x==0 && y==0) std::cout << "(0,0): r: " << (int)r << ", g: " << (int)g << ", b: " << (int)b << std::endl;
            }
         }
      }

      /*if(outputImage) {
        if(!osgDB::writeImageFile(*outputImage, "tiletest.jpg")) {
            std::cout << "Couldn't save output image!" << std::endl;
        }
        else {
            std::cout << "tiletest.jpg written!!!" << std::endl;
        }
      }*/

      fftw_execute(fftwForward1);
      fftw_execute(fftwForward2);

      double sigmax = 0.1;
      double sigmay = 0.1;
      double invSize = 1.0/(double)(inputImage->s()*inputImage->t());
                  
      em::mv<3,5,6>::type h(0.0,1.0,0.0);
      for(int x=0; x<inputImage->s(); ++x) {
         for(int y=0; y<inputImage->t(); ++y) {
            double s, t;
            if(x<=inputImage->s()/2) s=(double)x; else s=-(double)(inputImage->s()-x);
            if(y<=inputImage->t()/2) t=(double)y; else t=-(double)(inputImage->t()-y);

            double g = exp(-0.5*((s*s*sigmax*sigmax+t*t*sigmay*sigmay)));

            em::mv<0,3,5,6>::type f = (*(fftwFreq1+inputImage->t()*x+y))
               + part_type<em::mv<0>::type>(*(fftwFreq2+inputImage->t()*x+y))*e1*e3
               + part_type<em::mv<3>::type>(*(fftwFreq2+inputImage->t()*x+y));

            em::mv<0,3,5,6>::type r = f*g*h;

            (*(fftwFreq1+inputImage->t()*x+y)) = part_type<em::mv<0,6>::type>(r);
            (*(fftwFreq2+inputImage->t()*x+y)) = part_type<em::mv<5>::type>(r)*e3*e1 + part_type<em::mv<3>::type>(r);
         }
      }

      fftw_execute(fftwBackward1);
      fftw_execute(fftwBackward2);

      /*for(int x=0; x<inputImage->s(); ++x) {
         for(int y=0; y<inputImage->t(); ++y) {
            double r_double = (double)eval(part_type<em::mv<6>::type>(*(fftwImag1+inputImage->t()*x+y))*e3*e2)*invSize;
            double g_double = (double)eval((part_type<em::mv<0>::type>(*(fftwImag2+inputImage->t()*x+y))*e1*e3)*e1*e3)*invSize;
            double b_double = (double)eval(part_type<em::mv<3>::type>(*(fftwImag2+inputImage->t()*x+y))*e2*e1)*invSize;

            unsigned char& r = *(inputImage->data(x,y)+0);
            unsigned char& g = *(inputImage->data(x,y)+1);
            unsigned char& b = *(inputImage->data(x,y)+2);

            //r = (unsigned char)((double)eval(part_type<em::mv<6>::type>(*(fftwImag1+inputImage->t()*x+y))*e3*e2)*invSize);
            //g = (unsigned char)((double)eval((part_type<em::mv<0>::type>(*(fftwImag2+inputImage->t()*x+y))*e1*e3)*e1*e3)*invSize);
            //b = (unsigned char)((double)eval(part_type<em::mv<3>::type>(*(fftwImag2+inputImage->t()*x+y))*e2*e1)*invSize);
            
            //if(x==0 && y==0) std::cout << "\t(0,0): r: " << (int)r << ", g: " << (int)g << ", b: " << (int)b << std::endl;

            //if(r_double<0.0 || g_double<0.0 || b_double<0.0) {
            //   std::cout << "double: " << r_double << ", " << g_double << ", " << b_double << ", int: " << (int)r << ", " << (int)g << ", " << (int)b << std::endl;
            //}
         }
      }*/

      osg::Geometry* treeGeometry = new osg::Geometry;
      treeGeode->addDrawable(treeGeometry);

      osg::Vec3Array* treeVertices = new osg::Vec3Array;
      treeGeometry->setVertexArray(treeVertices);
      
      //osg::Vec3Array* treeNormals = new osg::Vec3Array;
	   //treeGeometry->setNormalArray(treeNormals);
	   //treeGeometry->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );

      osg::Vec2Array* treeTexCoords = new osg::Vec2Array;
      treeGeometry->setTexCoordArray(0, treeTexCoords);


      int nTrees = (int)((double)(field->getNumColumns()*field->getNumRows())*1e-2/tileElementArea);
      //std::cout << "nTrees: " << nTrees << std::endl;
      for(int i = 0; i<nTrees; ++i)
      {
         double x_mt = mt();
         //double x = x_mt*tileXInterval*(double)field->getNumColumns() + tileXMin + offset.x();
         double x = x_mt*tileXInterval*(double)field->getNumColumns() + tileXMin;
         int img_x = (int)(x_mt*(double)inputImage->s());
         double y_mt = mt();
         //double y = y_mt*tileYInterval*(double)field->getNumRows() + tileYMin + offset.y();
         double y = y_mt*tileYInterval*(double)field->getNumRows() + tileYMin;
         int img_y = (int)(y_mt*(double)inputImage->t());
         //std::cout << "x: " << x << ", img_x: " << img_x << ", y: " << y << ", img_y: " << img_y << std::endl;

         unsigned char r = (unsigned char)((double)eval(part_type<em::mv<6>::type>(*(fftwImag1+inputImage->t()*img_x+img_y))*e3*e2)*invSize);
         unsigned char g = (unsigned char)((double)eval((part_type<em::mv<0>::type>(*(fftwImag2+inputImage->t()*img_x+img_y))*e1*e3)*e1*e3)*invSize);
         unsigned char b = (unsigned char)((double)eval(part_type<em::mv<3>::type>(*(fftwImag2+inputImage->t()*img_x+img_y))*e2*e1)*invSize);
         if(r>200) {
            bool found = false;
            for(int rectIt = 0; rectIt < voidBoundingAreaVector.size(); ++rectIt)
            {
               if(voidBoundingAreaVector[rectIt].contains(osg::Vec2d(x, y))) {
                  std::cout << "Not foresting: x: " << x << ", y: " << y << std::endl;
                  found = true;
                  continue;
               }
            }
            if(found) continue;

            int x_int = floor((x-tileXMin)/tileXInterval+0.5);
            if(x_int<0) x_int=0; else if(x_int>=(int)field->getNumColumns()) x_int=field->getNumColumns()-1;
            int y_int = floor((y-tileYMin)/tileYInterval+0.5);
            if(y_int<0) y_int=0; else if((unsigned int)y_int>=field->getNumRows()) y_int=field->getNumRows()-1;
            double z = field->getHeight(x_int,y_int);

            /*osg::PositionAttitudeTransform* treeTrans = new osg::PositionAttitudeTransform();
            treeGroup->addChild(treeTrans);
            //std::cout << "Tree: (" << x-tileXMin << ", " << y-tileYMin << ", " << z << ")" << std::endl;

            osg::Vec3 treePos(x,y,z);
            treeTrans->setPosition(treePos - offset);
            int treeIt = (int)floor(mt()*(double)treeNodeVector.size());
            treeTrans->addChild(treeNodeVector[treeIt].get());
            treeTrans->setName("TreeTransform");*/


            treeVertices->push_back(osg::Vec3(x-3.18, y, z) - tileMin);
            //treeNormals->push_back(osg::Vec3(0.0, 1.0, 0.0));
            treeTexCoords->push_back(osg::Vec2(0.0, 0.0));
            treeVertices->push_back(osg::Vec3(x+3.18, y, z) - tileMin);
            //treeNormals->push_back(osg::Vec3(0.0, 1.0, 0.0));
            treeTexCoords->push_back(osg::Vec2(1.0, 0.0));
            treeVertices->push_back(osg::Vec3(x+3.18, y, z+10.0) - tileMin);
            //treeNormals->push_back(osg::Vec3(0.0, 1.0, 0.0));
            treeTexCoords->push_back(osg::Vec2(1.0, 1.0));
            treeVertices->push_back(osg::Vec3(x-3.18, y, z+10.0) - tileMin);
            //treeNormals->push_back(osg::Vec3(0.0, 1.0, 0.0));
            treeTexCoords->push_back(osg::Vec2(0.0, 1.0));

            treeVertices->push_back(osg::Vec3(x, y-3.18, z) - tileMin);
            treeTexCoords->push_back(osg::Vec2(0.0, 0.0));
            treeVertices->push_back(osg::Vec3(x, y+3.18, z) - tileMin);
            treeTexCoords->push_back(osg::Vec2(1.0, 0.0));
            treeVertices->push_back(osg::Vec3(x, y+3.18, z+10.0) - tileMin);
            treeTexCoords->push_back(osg::Vec2(1.0, 1.0));
            treeVertices->push_back(osg::Vec3(x, y-3.18, z+10.0) - tileMin);
            treeTexCoords->push_back(osg::Vec2(0.0, 1.0));

         }
      }
      //treeGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, baseIt, 4));
      treeGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, treeVertices->size()/4));


      fftw_destroy_plan(fftwForward1);
      fftw_destroy_plan(fftwForward2);
      fftw_destroy_plan(fftwBackward1);
      fftw_destroy_plan(fftwBackward2);

      fftw_free(fftwImag1);
      fftw_free(fftwImag2);
      fftw_free(fftwFreq1);
      fftw_free(fftwFreq2);

   }
#endif

    //tile->setDirty(true);
    //tile->dirtyBound();
}
