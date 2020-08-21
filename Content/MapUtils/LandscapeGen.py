# Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
#
#
#
# You need following modules to run this script
#   + pillow
#   + numpy
#   + gdal
#
# You can install these modules by running following commands on a posh prompt
#  PS> cd C:\Program Files\Epic Games\UE_4.25\Engine\Binaries\ThirdParty\Python\Win64
#  PS> ./python.exe -m pip install pillow numpy
# GDAL cannot install by this simple method. You need to download whl file from
#  https://www.lfd.uci.edu/~gohlke/pythonlibs/#gdal
# and then, install it by the similar command
#  PS> ./python.exe -m pip install /path/to/GDAL-2.2.4-cp27-cp27m-win_amd64.whl
#
#  You may want to add 
#   --target="C:\Program Files\Epic Games\UE_4.25\Engine\Source\ThirdParty\Python\Win64\Lib\site-packages"
#  to each pip install command. Without this --target option, modules will be installed in this folder.
#    C:\Program Files\Epic Games\UE_4.25\Engine\Binaries\ThirdParty\Python\Win64\Lib\site-packages

import unreal
import gdal
import osr
import os
from PIL import Image, ImageTransform, ImageMath
import numpy as np
import math

al=unreal.EditorAssetLibrary
el=unreal.EditorLevelLibrary
sdir=os.path.dirname(os.path.abspath(__file__))
projdir=os.path.join(sdir,"..","..","..","..")

# Parameters

stepSize=10  # [cm] # landscale cell size
zScale=10    # 100: +/-256 [m], 10: +/-25.6 [m]  1: 256 [cm]     # landscape z scale value
zEnhance=1   # optional z scaling factor
zOffset=None # None for z at GeoReference Actor
zClipping=19.0 # Clip lowest height [m]   (None for no clipping)
zClipping=-10.0 # Clip lowest height [m]   (None for no clipping)
inputGeotiff=os.path.join(projdir,"GeotiffDEM.tif")
inputGeotiff=os.path.join(projdir,"TC_P8_pede_surface_export.tif")
inputGeotiff=os.path.join(projdir,"TC_P8_pede_surface_export-median20_geo.tif")
outputHeightmap=os.path.join(projdir,"heightmap-premedian20.png")
toUEScale=100.*128./zScale   # [m]->[cm]->[heightmap unit]

LocalGeotiff=[
  {"file":os.path.join(projdir,"TC_P8_pede_elev_divmask-export.tif"),
  "zoffset":25.43}
]

# Utilities

# FVector  ([deg], [min], [sec]) -> float [deg]
def Decode60(vec):
  return ((vec.z/60.)+vec.y)/60.+vec.x;

# float [deg] -> FVector ([deg], [min], [sec])
def Encode60(v):
  d=math.floor(v)
  m=math.floor((v-d)*60.)
  s=(v-d-m/60.)*3600.
  return unreal.Vector(d,m,s)

class GeoTIFF:
  def __init__(self, file):
    self.gt = gdal.Open(file, gdal.GA_ReadOnly)
    #self.rast = np.array([self.gt.GetRasterBand(1).ReadAsArray()])
    self.image = Image.fromarray(self.gt.GetRasterBand(1).ReadAsArray())
    self.src_cs = osr.SpatialReference()
    self.dst_cs = osr.SpatialReference()
    self.dst_cs.ImportFromWkt(self.gt.GetProjectionRef())
    self.setSrcEPSG(6668)   # default to JGD2011

    # inverse transform from GeoTIFF(UV) to GeoTIFF(Logical)
    self.mat = self.gt.GetGeoTransform()
    d = 1./(self.mat[5]*self.mat[1]-self.mat[4]*self.mat[2])
    self.iaf = np.array([[ self.mat[5],-self.mat[2]],
                         [-self.mat[4], self.mat[1]]])*d
    self.offset = np.array([[self.mat[0]], [self.mat[3]]])
    self.af=np.array([[self.mat[1], self.mat[2]],
                      [self.mat[4], self.mat[5]]])

  def setSrcEPSG(self, epsg):
    self.src_cs = osr.SpatialReference()
    self.src_cs.ImportFromEPSG(epsg)
    self.transS2G = osr.CoordinateTransformation(self.src_cs, self.dst_cs)
    # Geotiff CS to Interface CS
    self.transG2S=osr.CoordinateTransformation(self.dst_cs,self.src_cs)

  def getBL(self,uv):
    u=uv[0]
    v=uv[1]
    bl=np.dot(self.af,np.array([[u],[v]]))+self.offset
    sbl=self.transG2S.TransformPoint(bl[1][0],bl[0][0])
    return (sbl[0],sbl[1])

  def getBBoxBL(self):
    # Geotiff CS to Interface CS
    return (self.getBL((0,0)),self.getBL((self.gt.RasterXSize,self.gt.RasterYSize)))

  def sanitizedBounds(self, bbox=None):
    if bbox is None:
      bbox=self.getBBoxBL()
    tl,br=bbox
    bmin, bmax = tl[0], br[0]
    if bmin>bmax:
      bmin, bmax = bmax, bmin
    lmin, lmax = tl[1], br[1]
    if lmin>lmax:
      lmin, lmax = lmax, lmin
    return ((bmin,bmax,lmin,lmax))

  def getIntersection(self, bboxBL):
    bbox=self.sanitizedBounds(bboxBL)
    sbbox=self.sanitizedBounds()
    bmin=max(bbox[0],sbbox[0])
    bmax=min(bbox[1],sbbox[1])
    lmin=max(bbox[2],sbbox[2])
    lmax=min(bbox[3],sbbox[3])
    if lmax < lmin or bmax < bmin:   # No intersection
      return None
    return ((bmax,lmin),(bmin,lmax))  # North-East, South-West

  def getUV(self, srcBL):
    gtBL = self.transS2G.TransformPoint(srcBL[1], srcBL[0])
    bl=np.array([[gtBL[0]],[gtBL[1]]])
    uv = np.dot(self.iaf, bl-self.offset)
    return (uv[0][0], uv[1][0])

def getLandscapeBBox():
  # search for landscape proxy actors
  w=el.get_all_level_actors()
  theFirst=True
  for a in w:
    if(a.get_class().get_name().startswith("Landscape") and not a.get_class().get_name().startswith("LandscapeGizmo")):
      #print("Landscape Found : "+ a.get_name())
      o,box=a.get_actor_bounds(True)
      h=o+box
      l=o-box
      if(theFirst):
        lx=l.x
        ly=l.y
        hx=h.x
        hy=h.y
        theFirst=False
      else:
        lx=min(lx,l.x)
        ly=min(ly,l.y)
        hx=max(hx,h.x)
        hy=max(hy,h.y)
  print("Landscape bounding box: ({0}, {1}  -  {2}, {3})".format(lx,ly,hx,hy))
  print("Landscape size: {0} x {1}".format(hx-lx,hy-ly))
  size=(int((hx-lx)/stepSize+1),int((hy-ly)/stepSize+1))
  print("Landscape grid size: {0}".format(size))
  return (lx,ly,hx,hy,size)

def getGeoReference():
  w=el.get_all_level_actors()
  theFirst=True
  for a in w:
    if(a.get_class().get_name().startswith("GeoReferenceBP")):
      print("GeoReference Found")
      ref=a
  ref.initialize_geo_conv()
  return ref

# ----------------------------------------------------------------------------------------

lx,ly,hx,hy,size=getLandscapeBBox()
ref=getGeoReference()

text_label="Projecting coordinates"
nFrames=5
with unreal.ScopedSlowTask(nFrames, text_label) as slow_task:
  slow_task.make_dialog(True)

  tl=tuple(map(Decode60, ref.get_bl(lx,ly)))
  bl=tuple(map(Decode60, ref.get_bl(lx,hy)))
  br=tuple(map(Decode60, ref.get_bl(hx,hy)))
  tr=tuple(map(Decode60, ref.get_bl(hx,ly)))
  print("Reference Quad=tl:{0} bl:{1} br:{2} tr:{3}".format(tl, bl, br, tr))
  zo=ref.get_actor_location()
  zobl=tuple(map(Decode60,ref.get_bl(zo.x,zo.y)))
  print("GeoReference in BL {0} {1}".format(zobl[0], zobl[1]))
  print("GeoReference in UE {0}".format(zo))
  print(ref.get_xy(Encode60(zobl[0]),Encode60(zobl[1])))

  gt=GeoTIFF(inputGeotiff)
  tluv=gt.getUV(tl)
  bluv=gt.getUV(bl)
  bruv=gt.getUV(br)
  truv=gt.getUV(tr)
  zouv=gt.getUV(zobl)

  print("Reference Quad on GeoTIFF image =tl:{0} bl:{1} br:{2} tr:{3}".format(tluv, bluv, bruv, truv))
  uvf=tluv+bluv+bruv+truv

  tlbl=gt.getBL(tluv)
  blbl=gt.getBL(bluv)
  trbl=gt.getBL(truv)
  brbl=gt.getBL(bruv)

  print("Reference Quad returned =tl:{0} bl:{1} br:{2} tr:{3}".format(tlbl, blbl, brbl, trbl))
  print("Geotiff BBox = {0}".format(gt.getBBoxBL()))

  slow_task.enter_progress_frame(1,"Clipping z range")
  print(gt.image.mode)
  print(gt.image)
  if zClipping is not None:
    imageref=Image.new(gt.image.mode,gt.image.size,zClipping)
    clippedimg=ImageMath.eval("max(a,b)",a=gt.image,b=imageref)
    clippedimg.save(os.path.join(projdir,"Assets","clipped.tif"))
  else:
    clippedimg=gt.image

  print("Geotiff bounds:{0}".format(gt.getBBoxBL()))
  for lg in LocalGeotiff:
    lgt=GeoTIFF(lg["file"])
    print("Local Geotiff bounds:{0}".format(lgt.getBBoxBL()))
    isc=gt.getIntersection(lgt.getBBoxBL())
    print("{0} intersection {1} ".format(lg["file"],isc))

  if True:
    slow_task.enter_progress_frame(1,"Transforming image region")

    img=clippedimg.transform(size,Image.QUAD,data=uvf,resample=Image.BICUBIC)
    img.save(os.path.join(projdir,"transform.tif"))

    slow_task.enter_progress_frame(1,"Transforming height values")

    # scale to match landscape scaling, and offset to align to GeoReference actor
    if zOffset is None:
      print(zouv)
      zou=min(max(zouv[0],0),clippedimg.size[0])
      zov=min(max(zouv[1],0),clippedimg.size[1])
      zoff=clippedimg.getpixel((zou,zov))
    else:
      zoff=zOffset
    zos=32768-(zoff*zEnhance-zo.z/100.)*toUEScale   # 32768: mid point (height=0)
    iarrf=np.array(img.getdata(),dtype="float32")*toUEScale*zEnhance + zos
    print(zov)
    print(zos)
    print(zEnhance)
    print(toUEScale)
    print(iarrf.dtype)
    print(iarrf)
    slow_task.enter_progress_frame(1,"Converting to 16bit grayscale")
    # convert to uint16 using numpy
    # PIL cannot handle this operation because of CLIP16() which limits each pixel value in -32768 to 32767.
    # This clipping must be avoided because destination dtype is uint16.
    iarrs=np.array(iarrf,dtype="uint16")

    slow_task.enter_progress_frame(1,"Saving as {0}".format(os.path.basename(outputHeightmap)))

    imgS=Image.frombuffer("I;16",img.size,iarrs.data, "raw", "I;16", 0, 1)
    imgS.save(outputHeightmap)
    print("Heightmap saved as {0}".format(outputHeightmap))
