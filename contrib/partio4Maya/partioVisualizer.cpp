/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Visualizer
Copyright 2013 (c)  All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

Disclaimer: THIS SOFTWARE IS PROVIDED BY  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#include "partioVisualizer.h"

static MGLFunctionTable *gGLFT = NULL;


// id is registered with autodesk no need to change
#define ID_PARTIOVISUALIZER  0x00116ECF

#define LEAD_COLOR				18	// green
#define ACTIVE_COLOR			15	// white
#define ACTIVE_AFFECTED_COLOR	8	// purple
#define DORMANT_COLOR			4	// blue
#define HILITE_COLOR			17	// pale blue

#define DRAW_STYLE_POINTS 0
#define DRAW_STYLE_RADIUS 1
#define DRAW_STYLE_DISK	 2
#define DRAW_STYLE_BOUNDING_BOX 3

using namespace Partio;
using namespace std;

/// ///////////////////////////////////////////////////
/// PARTIO VISUALIZER

MTypeId partioVisualizer::id( ID_PARTIOVISUALIZER );

MObject partioVisualizer::time;
MObject partioVisualizer::aByFrame;
MObject partioVisualizer::aDrawSkip;
MObject partioVisualizer::aUpdateCache;
MObject partioVisualizer::aSize;         // The size of the logo
MObject partioVisualizer::aFlipYZ;
MObject partioVisualizer::aCacheDir;
MObject partioVisualizer::aCacheFile;
MObject partioVisualizer::aUseTransform;
MObject partioVisualizer::aCacheActive;
MObject partioVisualizer::aCacheOffset;
MObject partioVisualizer::aCacheStatic;
MObject partioVisualizer::aCacheFormat;
MObject partioVisualizer::aJitterPos;
MObject partioVisualizer::aJitterFreq;
MObject partioVisualizer::aPartioAttributes;
MObject partioVisualizer::aColorFrom;
MObject partioVisualizer::aRadiusFrom;
MObject partioVisualizer::aAlphaFrom;
MObject partioVisualizer::aIncandFrom;
MObject partioVisualizer::aPointSize;
MObject partioVisualizer::aDefaultPointColor;
MObject partioVisualizer::aDefaultAlpha;
MObject partioVisualizer::aDefaultRadius;
MObject partioVisualizer::aInvertAlpha;
MObject partioVisualizer::aDrawStyle;
MObject partioVisualizer::aForceReload;
MObject partioVisualizer::aRenderCachePath;

MString partioVisualizer::drawDbClassification("drawdb/geometry/partio/visualizer");

namespace {
    void drawBillboardCircleAtPoint(const float* position, float radius, int num_segments, int drawType)
    {
        glPushMatrix();
        glTranslatef(position[0], position[1], position[2]);
        glMatrixMode(GL_MODELVIEW_MATRIX);
        float m[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, m);
        m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f;
        m[4] = 0.0f; m[5] = 1.0f; m[6] = 0.0f;
        m[8] = 0.0f; m[9] = 0.0f; m[10] = 1.0f;
        glLoadMatrixf(m);

        float theta =(float)(  2 * 3.1415926 / float(num_segments)  );
        float tangetial_factor = tanf(theta);//calculate the tangential factor

        float radial_factor = cosf(theta);//calculate the radial factor

        float x = radius;//we start at angle = 0
        float y = 0;

        if (drawType == 1)
        {
            glBegin(GL_LINE_LOOP);
        }
        else if (drawType == 2)
        {
            glBegin(GL_POLYGON);
        }
        for (int ii = 0; ii < num_segments; ii++)
        {
            glVertex2f(x, y);//output vertex

            //calculate the tangential vector
            //remember, the radial vector is (x, y)
            //to get the tangential vector we flip those coordinates and negate one of them

            float tx = -y;
            float ty = x;

            //add the tangential vector

            x += tx * tangetial_factor;
            y += ty * tangetial_factor;

            //correct using the radial factor

            x *= radial_factor;
            y *= radial_factor;
        }
        glEnd();
        glTranslatef(-position[0], -position[1], -position[2]);
        glPopMatrix();
    }
}


partioVizReaderCache::partioVizReaderCache():
        bbox(MBoundingBox(MPoint(0,0,0,0),MPoint(0,0,0,0))),
        dList(0),
        particles(NULL)
{
}


/// CREATOR
partioVisualizer::partioVisualizer()
        :   mLastFileLoaded(""),
        mLastAlpha(0.0),
        mLastInvertAlpha(false),
        mLastPath(""),
        mLastFile(""),
        mLastExt(""),
        mLastColorFromIndex(-1),
        mLastAlphaFromIndex(-1),
        mLastRadiusFromIndex(-1),
        mLastIncandFromIndex(-1),
        mLastColor(1,0,0),
        mLastRadius(1.0),
        cacheChanged(false),
        mFrameChanged(false),
        multiplier(1.0),
        mFlipped(false),
        drawError(0)
{
    pvCache.particles = NULL;
}
/// DESTRUCTOR
partioVisualizer::~partioVisualizer()
{
    if (pvCache.particles)
        pvCache.particles->release();
    MSceneMessage::removeCallback(partioVisualizerOpenCallback);
    MSceneMessage::removeCallback(partioVisualizerImportCallback);
    MSceneMessage::removeCallback(partioVisualizerReferenceCallback);
}

void* partioVisualizer::creator()
{
    return new partioVisualizer;
}

/// POST CONSTRUCTOR
void partioVisualizer::postConstructor()
{
    setRenderable(true);
    partioVisualizerOpenCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, partioVisualizer::reInit, this);
    partioVisualizerImportCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, partioVisualizer::reInit, this);
    partioVisualizerReferenceCallback = MSceneMessage::addCallback(MSceneMessage::kAfterReference, partioVisualizer::reInit, this);
}

///////////////////////////////////
/// init after opening
///////////////////////////////////

void partioVisualizer::initCallback()
{

    MObject tmo = thisMObject();

    short extENum;
    MPlug(tmo, aCacheFormat).getValue(extENum);

    mLastExt = partio4Maya::setExt(extENum);

    MPlug(tmo,aCacheDir).getValue(mLastPath);
    MPlug(tmo,aCacheFile).getValue(mLastFile);
    MPlug(tmo,aDefaultAlpha).getValue(mLastAlpha);
    MPlug(tmo,aDefaultRadius).getValue(mLastRadius);
    MPlug(tmo,aColorFrom).getValue(mLastColorFromIndex);
    MPlug(tmo,aAlphaFrom).getValue(mLastAlphaFromIndex);
    MPlug(tmo,aRadiusFrom).getValue(mLastRadiusFromIndex);
	MPlug(tmo,aIncandFrom).getValue(mLastIncandFromIndex);
    MPlug(tmo,aSize).getValue(multiplier);
    MPlug(tmo,aInvertAlpha).getValue(mLastInvertAlpha);
    MPlug(tmo,aCacheStatic).getValue(mLastStatic);
    cacheChanged = false;

}

void partioVisualizer::reInit(void *data)
{
    partioVisualizer  *vizNode = (partioVisualizer*) data;
    vizNode->initCallback();
}


MStatus partioVisualizer::initialize()
{

    MFnEnumAttribute 	eAttr;
    MFnUnitAttribute 	uAttr;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute 	tAttr;
    MStatus				stat;

    time = nAttr.create("time", "tm", MFnNumericData::kLong ,0);
    uAttr.setKeyable(true);

	aByFrame = nAttr.create( "byFrame", "byf", MFnNumericData::kInt ,1);
    nAttr.setKeyable(true);
    nAttr.setReadable(true);
    nAttr.setWritable(true);
    nAttr.setConnectable(true);
    nAttr.setStorable(true);
    nAttr.setMin(1);
    nAttr.setMax(100);

    aSize = uAttr.create( "iconSize", "isz", MFnUnitAttribute::kDistance );
    uAttr.setDefault( 0.25 );

    aFlipYZ = nAttr.create( "flipYZ", "fyz", MFnNumericData::kBoolean);
    nAttr.setDefault ( false );
    nAttr.setKeyable ( true );

    aDrawSkip = nAttr.create( "drawSkip", "dsk", MFnNumericData::kLong ,0);
    nAttr.setKeyable( true );
    nAttr.setReadable( true );
    nAttr.setWritable( true );
    nAttr.setConnectable( true );
    nAttr.setStorable( true );
    nAttr.setMin(0);
    nAttr.setMax(1000);

    aCacheDir = tAttr.create ( "cacheDir", "cachD", MFnStringData::kString );
    tAttr.setReadable ( true );
    tAttr.setWritable ( true );
    tAttr.setKeyable ( true );
    tAttr.setConnectable ( true );
    tAttr.setStorable ( true );

    aCacheFile = tAttr.create ( "cachePrefix", "cachP", MFnStringData::kString );
    tAttr.setReadable ( true );
    tAttr.setWritable ( true );
    tAttr.setKeyable ( true );
    tAttr.setConnectable ( true );
    tAttr.setStorable ( true );

    aCacheOffset = nAttr.create("cacheOffset", "coff", MFnNumericData::kInt, 0, &stat );
    nAttr.setKeyable(true);

    aCacheStatic = nAttr.create("staticCache", "statC", MFnNumericData::kBoolean, false, &stat);
    nAttr.setKeyable(true);

    aCacheActive = nAttr.create("cacheActive", "cAct", MFnNumericData::kBoolean, 1, &stat);
    nAttr.setKeyable(true);


    aCacheFormat = eAttr.create( "cacheFormat", "cachFmt");
    std::map<short,MString> formatExtMap;
    partio4Maya::buildSupportedExtensionList(formatExtMap,false);
    for (unsigned short i = 0; i< formatExtMap.size(); i++)
    {
        eAttr.addField(formatExtMap[i].toUpperCase(),	i);
    }

    eAttr.setDefault(4);  // PDC
    eAttr.setChannelBox(true);

    aDrawStyle = eAttr.create( "drawStyle", "drwStyl");
    eAttr.addField("points",	0);
    eAttr.addField("radius", 1);
    eAttr.addField("disk", 2);
    eAttr.addField("boundingBox", 3);
    //eAttr.addField("sphere", 4);
    //eAttr.addField("velocity", 5);

    eAttr.setDefault(0);
    eAttr.setChannelBox(true);


    aUseTransform = nAttr.create("useTransform", "utxfm", MFnNumericData::kBoolean, false, &stat);
    nAttr.setKeyable(true);

    aPartioAttributes = tAttr.create ("partioCacheAttributes", "pioCAts", MFnStringData::kString);
    tAttr.setArray(true);
    tAttr.setUsesArrayDataBuilder( true );

    aColorFrom = nAttr.create("colorFrom", "cfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aAlphaFrom = nAttr.create("opacityFrom", "ofrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);
	
	aIncandFrom= nAttr.create("incandescenceFrom", "ifrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aRadiusFrom = nAttr.create("radiusFrom", "rfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aPointSize = nAttr.create("pointSize", "ptsz", MFnNumericData::kInt, 2, &stat);
    nAttr.setDefault(2);
    nAttr.setKeyable(true);

    aDefaultPointColor = nAttr.createColor("defaultPointColor", "dpc", &stat);
    nAttr.setDefault(1.0f, 0.0f, 0.0f);
    nAttr.setKeyable(true);

    aDefaultAlpha = nAttr.create("defaultAlpha", "dalf", MFnNumericData::kFloat, 1.0, &stat);
    nAttr.setDefault(1.0);
    nAttr.setMin(0.0);
    nAttr.setMax(1.0);
    nAttr.setKeyable(true);

    aInvertAlpha= nAttr.create("invertAlpha", "ialph", MFnNumericData::kBoolean, false, &stat);
    nAttr.setDefault(false);
    nAttr.setKeyable(true);

    aDefaultRadius = nAttr.create("defaultRadius", "drad", MFnNumericData::kFloat, 1.0, &stat);
    nAttr.setDefault(1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);

    aForceReload = nAttr.create("forceReload", "frel", MFnNumericData::kBoolean, false, &stat);
    nAttr.setDefault(false);
    nAttr.setKeyable(false);

    aUpdateCache = nAttr.create("updateCache", "upc", MFnNumericData::kInt, 0);
    nAttr.setHidden(true);

    aRenderCachePath = tAttr.create ( "renderCachePath", "rcp", MFnStringData::kString );
    nAttr.setHidden(true);


    addAttribute(aUpdateCache);
    addAttribute(aSize);
    addAttribute(aFlipYZ);
    addAttribute(aDrawSkip);
    addAttribute(aCacheDir);
    addAttribute(aCacheFile);
    addAttribute(aCacheOffset);
    addAttribute(aCacheStatic);
    addAttribute(aCacheActive);
    addAttribute(aCacheFormat);
    addAttribute(aPartioAttributes);
    addAttribute(aColorFrom);
	addAttribute(aIncandFrom);
    addAttribute(aAlphaFrom);
    addAttribute(aRadiusFrom);
    addAttribute(aPointSize);
    addAttribute(aDefaultPointColor);
    addAttribute(aDefaultAlpha);
    addAttribute(aInvertAlpha);
    addAttribute(aDefaultRadius);
    addAttribute(aDrawStyle);
    addAttribute(aForceReload);
    addAttribute(aRenderCachePath);
	addAttribute(aByFrame);
    addAttribute(time);

	attributeAffects(aCacheActive, aUpdateCache);
    attributeAffects(aCacheDir, aUpdateCache);
    attributeAffects(aSize, aUpdateCache);
    attributeAffects(aFlipYZ, aUpdateCache);
    attributeAffects(aCacheFile, aUpdateCache);
    attributeAffects(aCacheOffset, aUpdateCache);
    attributeAffects(aCacheStatic, aUpdateCache);
    attributeAffects(aCacheFormat, aUpdateCache);
    attributeAffects(aColorFrom, aUpdateCache);
	attributeAffects(aIncandFrom, aUpdateCache);
    attributeAffects(aAlphaFrom, aUpdateCache);
    attributeAffects(aRadiusFrom, aUpdateCache);
    attributeAffects(aPointSize, aUpdateCache);
    attributeAffects(aDefaultPointColor, aUpdateCache);
    attributeAffects(aDefaultAlpha, aUpdateCache);
    attributeAffects(aInvertAlpha, aUpdateCache);
    attributeAffects(aDefaultRadius, aUpdateCache);
    attributeAffects(aDrawStyle, aUpdateCache);
    attributeAffects(aForceReload, aUpdateCache);
	attributeAffects(aByFrame, aUpdateCache);
	attributeAffects(aByFrame, aRenderCachePath);
	attributeAffects(time, aUpdateCache);
    attributeAffects(time,aRenderCachePath);


    return MS::kSuccess;
}


partioVizReaderCache* partioVisualizer::updateParticleCache()
{
    GetPlugData(); // force update to run compute function where we want to do all the work
    return &pvCache;
}

// COMPUTE FUNCTION

MStatus partioVisualizer::compute( const MPlug& plug, MDataBlock& block )
{

    int colorFromIndex  = block.inputValue( aColorFrom ).asInt();
	int incandFromIndex = block.inputValue( aIncandFrom ).asInt();
    int opacityFromIndex= block.inputValue( aAlphaFrom ).asInt();
    int radiusFromIndex = block.inputValue( aRadiusFrom ).asInt();
    bool cacheActive = block.inputValue(aCacheActive).asBool();


    // Determine if we are requesting the output plug for this node.
    //
    if (plug != aUpdateCache)
    {
        return ( MS::kUnknownParameter );
    }

    else
    {
        MStatus stat;

        MString cacheDir 	= block.inputValue(aCacheDir).asString();
        MString cacheFile = block.inputValue(aCacheFile).asString();

        drawError = 0;
        if (cacheDir  == "" || cacheFile == "" )
        {
            drawError = 1;
            // too much printing  rather force draw of icon to red or something
            //MGlobal::displayError("PartioVisualizer->Error: Please specify cache file!");
            return ( MS::kFailure );
        }

        bool cacheStatic			= block.inputValue( aCacheStatic ).asBool();
        int cacheOffset 			= block.inputValue( aCacheOffset ).asInt();
        short cacheFormat			= block.inputValue( aCacheFormat ).asShort();
        MFloatVector defaultColor 	= block.inputValue( aDefaultPointColor ).asFloatVector();
        float defaultAlpha 			= block.inputValue( aDefaultAlpha ).asFloat();
        bool invertAlpha 			= block.inputValue( aInvertAlpha ).asBool();
        float defaultRadius			= block.inputValue( aDefaultRadius).asFloat();
        bool forceReload 			= block.inputValue( aForceReload ).asBool();
        int integerTime				= block.inputValue(time).asInt();
		int byFrame 				= block.inputValue( aByFrame ).asInt();
        bool flipYZ 				= block.inputValue( aFlipYZ ).asBool();
        MString renderCachePath 	= block.inputValue( aRenderCachePath ).asString();

        MString formatExt = "";
        int cachePadding = 0;

        MString newCacheFile = "";
        MString renderCacheFile = "";

        partio4Maya::updateFileName( cacheFile,  cacheDir,
                                     cacheStatic,  cacheOffset,
                                     cacheFormat,  integerTime, byFrame,
                                     cachePadding, formatExt,
                                     newCacheFile, renderCacheFile
                                   );

        if (renderCachePath != renderCacheFile || forceReload )
        {
            block.outputValue(aRenderCachePath).setString(renderCacheFile);
        }

        cacheChanged = false;

//////////////////////////////////////////////
/// Cache can change manually by changing one of the parts of the cache input...
        if (mLastExt != formatExt ||
                mLastPath != cacheDir ||
                mLastFile != cacheFile ||
                mLastFlipStatus  != flipYZ ||
                mLastStatic !=  cacheStatic ||
                forceReload )
        {
            cacheChanged = true;
            mFlipped = false;
            mLastFlipStatus = flipYZ;
            mLastExt = formatExt;
            mLastPath = cacheDir;
            mLastFile = cacheFile;
            mLastStatic = cacheStatic;
            block.outputValue(aForceReload).setBool(false);
        }

//////////////////////////////////////////////
/// or it can change from a time input change


        if (!partio4Maya::partioCacheExists(newCacheFile.asChar()))
        {
			if (pvCache.particles != NULL)
			{
				ParticlesDataMutable* newParticles;
				newParticles = pvCache.particles;
				pvCache.particles=NULL; // resets the particles

				if (newParticles != NULL)
				{
					//cout <<  "releasing" << endl;
					newParticles->release();
				}
			}
            pvCache.bbox.clear();
			mLastFileLoaded = "";
			drawError = 1;
        }

        //  after updating all the file path stuff,  exit here if we don't want to actually load any new data
		if (!cacheActive)
		{
			forceReload = true;
			if (pvCache.particles != NULL)
			{
				ParticlesDataMutable* newParticles;
				newParticles = pvCache.particles;
				pvCache.particles=NULL; // resets the pointer

				if (newParticles != NULL)
				{
					newParticles->release();
				}
			}
            pvCache.bbox.clear();
			mLastFileLoaded = "";
			drawError = 2;
			return ( MS::kSuccess );
		}

        if ( newCacheFile != "" &&
                partio4Maya::partioCacheExists(newCacheFile.asChar()) &&
                (newCacheFile != mLastFileLoaded || forceReload)
           )
        {
            cacheChanged = true;
            mFlipped = false;
            MGlobal::displayWarning(MString("PartioVisualizer->Loading: " + newCacheFile));

			if(pvCache.particles != NULL)
			{
				/////////////////////////////////////////////
				/// This seems to work to solve the mem leak
				ParticlesDataMutable* newParticles;
				newParticles = pvCache.particles;
				pvCache.particles=NULL; // resets the pointer

				if (newParticles != NULL)
				{
					newParticles->release(); // frees the mem
				}
			}
			pvCache.particles = read(newCacheFile.asChar());
			///////////////////////////////////////

            mLastFileLoaded = newCacheFile;
            if (pvCache.particles->numParticles() == 0)
            {
                return (MS::kSuccess);
            }

            char partCount[50];
            sprintf (partCount, "%d", pvCache.particles->numParticles());
            MGlobal::displayInfo(MString ("PartioVisualizer-> LOADED: ") + partCount + MString (" particles"));

            try{
                pvCache.rgba.resize(pvCache.particles->numParticles() * 4ul);
            }
            catch(...)
            {
                MGlobal::displayError("PartioVisualizer->unable to allocate new memory for particles");
                return (MS::kFailure);
            }

            pvCache.radius.clear();
            pvCache.radius.setLength(pvCache.particles->numParticles());

            pvCache.bbox.clear();

            if (!pvCache.particles->attributeInfo("position",pvCache.positionAttr) &&
                    !pvCache.particles->attributeInfo("Position",pvCache.positionAttr))
            {
                MGlobal::displayError("PartioVisualizer->Failed to find position attribute ");
                return ( MS::kFailure );
            }
            else
            {
                /// TODO: possibly move this down to take into account the radius as well for a true bounding volume
                // resize the bounding box
                for (int i = 0; i < pvCache.particles->numParticles(); ++i)
                {
                    const float* partioPositions = pvCache.particles->data<float>(pvCache.positionAttr, i);
                    MPoint pos(partioPositions[0], partioPositions[1], partioPositions[2]);
                    pvCache.bbox.expand(pos);
                }
            }

            block.outputValue(aForceReload).setBool(false);
            block.setClean(aForceReload);

        }

        if (pvCache.particles)
        {
            // something changed..
            if  (cacheChanged || mLastColorFromIndex != colorFromIndex || mLastColor != defaultColor)
            {
                int numAttrs = pvCache.particles->numAttributes();
                if (colorFromIndex+1 > numAttrs || opacityFromIndex+1 > numAttrs || radiusFromIndex+1 > numAttrs || incandFromIndex+1 > numAttrs)
                {
                    // reset the attrs
                    block.outputValue(aColorFrom).setInt(-1);
                    block.setClean(aColorFrom);
                    block.outputValue(aAlphaFrom).setInt(-1);
                    block.setClean(aAlphaFrom);
                    block.outputValue(aRadiusFrom).setInt(-1);
                    block.setClean(aRadiusFrom);
					block.outputValue(aIncandFrom).setInt(-1);
					block.setClean(aIncandFrom);

                    colorFromIndex  = block.inputValue( aColorFrom ).asInt();
                    opacityFromIndex= block.inputValue( aAlphaFrom ).asInt();
                    radiusFromIndex = block.inputValue( aRadiusFrom ).asInt();
					incandFromIndex = block.inputValue( aIncandFrom ).asInt();
                }

                if (colorFromIndex >=0)
                {
                    pvCache.particles->attributeInfo(colorFromIndex,pvCache.colorAttr);
                    // VECTOR or  4+ element float attrs
                    if (pvCache.colorAttr.type == VECTOR || pvCache.colorAttr.count > 2) // assuming first 3 elements are rgb
                    {
                        for (int i = 0;i < pvCache.particles->numParticles(); ++i)
                        {
                            const float * attrVal = pvCache.particles->data<float>(pvCache.colorAttr, i);
                            pvCache.rgba[i * 4]     = attrVal[0];
                            pvCache.rgba[i * 4 + 1] = attrVal[1];
                            pvCache.rgba[i * 4 + 2] = attrVal[2];
                        }
                    }
                    else // single FLOAT
                    {
                        for (int i = 0; i < pvCache.particles->numParticles(); ++i)
                        {
                            const float* attrVal = pvCache.particles->data<float>(pvCache.colorAttr, i);
                            pvCache.rgba[i * 4] = pvCache.rgba[i * 4 + 1] = pvCache.rgba[i * 4 + 2] = attrVal[0];
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < pvCache.particles->numParticles(); ++i)
                    {
                        pvCache.rgba[i * 4]     = defaultColor[0];
                        pvCache.rgba[i * 4 + 1] = defaultColor[1];
                        pvCache.rgba[i * 4 + 2] = defaultColor[2];
                    }

                }
                mLastColorFromIndex = colorFromIndex;
                mLastColor = defaultColor;
            }

            if  (cacheChanged || opacityFromIndex != mLastAlphaFromIndex || defaultAlpha != mLastAlpha || invertAlpha != mLastInvertAlpha)
            {
                if (opacityFromIndex >=0)
                {
                    pvCache.particles->attributeInfo(opacityFromIndex,pvCache.opacityAttr);
                    if (pvCache.opacityAttr.count == 1)  // single float value for opacity
                    {
                        for (int i = 0; i < pvCache.particles->numParticles(); ++i)
                        {
                            const float* attrVal = pvCache.particles->data<float>(pvCache.opacityAttr, i);
                            pvCache.rgba[i * 4 + 3] = invertAlpha ? 1.0f - attrVal[0] : attrVal[0];
                        }
                    }
                    else
                    {
                        if (pvCache.opacityAttr.count == 4)   // we have an  RGBA 4 float attribute ?
                        {
                            for (int i = 0; i < pvCache.particles->numParticles(); ++i)
                            {
                                const float* attrVal = pvCache.particles->data<float>(pvCache.opacityAttr, i);
                                pvCache.rgba[i * 4 + 3] = invertAlpha ? 1.0f - attrVal[3] : attrVal[3];
                            }
                        }
                        else
                        {
                            for (int i = 0; i < pvCache.particles->numParticles(); ++i)
                            {
                                const float* attrVal = pvCache.particles->data<float>(pvCache.opacityAttr, i);
                                const float lum = attrVal[0] * 0.2126f + attrVal[1] * 0.7152f + attrVal[2] * .0722f;
                                pvCache.rgba[i * 4 + 3] = invertAlpha ? 1.0f - lum : lum;
                            }
                        }
                    }
                }
                else
                {
                    mLastAlpha = invertAlpha ? 1.0f - defaultAlpha : defaultAlpha;
                    for (int i = 0; i < pvCache.particles->numParticles(); ++i)
                        pvCache.rgba[i * 4 + 3] = mLastAlpha;
                }
                mLastAlpha = defaultAlpha;
                mLastAlphaFromIndex = opacityFromIndex;
                mLastInvertAlpha = invertAlpha;
            }

            if (cacheChanged || radiusFromIndex != mLastRadiusFromIndex || defaultRadius != mLastRadius )
            {
                if (radiusFromIndex >= 0)
                {
                    pvCache.particles->attributeInfo(radiusFromIndex,pvCache.radiusAttr);
                    if (pvCache.radiusAttr.count == 1)  // single float value for radius
                    {
                        for (int i = 0; i < pvCache.particles->numParticles(); ++i)
                        {
                            const float* attrVal = pvCache.particles->data<float>(pvCache.radiusAttr, i);
                            pvCache.radius[i] = attrVal[0] * defaultRadius;
                        }
                    }
                    else
                    {
                        for (int i = 0; i < pvCache.particles->numParticles(); ++i)
                        {
                            const float* attrVal = pvCache.particles->data<float>(pvCache.radiusAttr, i);
                            float lum = attrVal[0] * 0.2126f + attrVal[1] * 0.7152f + attrVal[2] * .0722f;
                            pvCache.radius[i] = std::max(0.0f, lum * defaultRadius);
                        }

                    }
                }
                else
                {
                    mLastRadius = defaultRadius;
                    for (int i = 0; i < pvCache.particles->numParticles(); ++i)
                        pvCache.radius[i] = mLastRadius;
                }
                mLastRadius = defaultRadius;
                mLastRadiusFromIndex = radiusFromIndex;
            }
            if  (cacheChanged || incandFromIndex != mLastIncandFromIndex ) // incandescence does not affect viewport draw for now
                mLastIncandFromIndex = incandFromIndex;
        }
    }


    if (pvCache.particles) // update the AE Controls for attrs in the cache
    {
        unsigned int numAttr=pvCache.particles->numAttributes();
        MPlug zPlug (thisMObject(), aPartioAttributes);

        if ((colorFromIndex + 1) > (int)zPlug.numElements())
        {
            block.outputValue(aColorFrom).setInt(-1);
        }
        if ((opacityFromIndex + 1) > (int)zPlug.numElements())
        {
            block.outputValue(aAlphaFrom).setInt(-1);
        }
        if ((radiusFromIndex + 1) > (int)zPlug.numElements())
        {
            block.outputValue(aRadiusFrom).setInt(-1);
        }
        if ((incandFromIndex + 1) > (int)zPlug.numElements())
        {
            block.outputValue(aIncandFrom).setInt(-1);
        }

        if (cacheChanged || zPlug.numElements() != numAttr) // update the AE Controls for attrs in the cache
        {
            attributeList.clear();

            for (unsigned int i = 0; i < numAttr; ++i)
            {
                ParticleAttribute attr;
                pvCache.particles->attributeInfo(i,attr);

                // crazy casting string to  char
                char *temp;
                temp = new char[(attr.name).length()+1];
                strcpy (temp, attr.name.c_str());

                MString  mStringAttrName("");
                mStringAttrName += MString(temp);

                zPlug.selectAncestorLogicalIndex(i,aPartioAttributes);
                zPlug.setValue(MString(temp));
                attributeList.append(mStringAttrName);

                delete[] temp;
            }

            MArrayDataHandle hPartioAttrs = block.inputArrayValue(aPartioAttributes);
            MArrayDataBuilder bPartioAttrs = hPartioAttrs.builder();
            // do we need to clean up some attributes from our array?
            if (bPartioAttrs.elementCount() > numAttr)
            {
                unsigned int current = bPartioAttrs.elementCount();
                //unsigned int attrArraySize = current - 1;

                // remove excess elements from the end of our attribute array
                for (unsigned int x = numAttr; x < current; x++)
                {
                    bPartioAttrs.removeElement(x);
                }
            }
        }
    }
    block.setClean(plug);
    return MS::kSuccess;
}


/////////////////////////////////////////////////////
/// procs to override bounding box mode...
bool partioVisualizer::isBounded() const
{
    return true;
}

MBoundingBox partioVisualizer::boundingBox() const
{
    // Returns the bounding box for the shape.
    partioVisualizer* nonConstThis = const_cast<partioVisualizer*>(this);
    partioVizReaderCache* geom = nonConstThis->updateParticleCache();
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
    MPoint corner1 = geom->bbox.min();
    MPoint corner2 = geom->bbox.max();
    return MBoundingBox( corner1, corner2 );
}

//
// Select function. Gets called when the bbox for the object is selected.
// This function just selects the object without doing any intersection tests.
//
bool partioVisualizerUI::select( MSelectInfo &selectInfo,
                                 MSelectionList &selectionList,
                                 MPointArray &worldSpaceSelectPts ) const
{
    MSelectionMask priorityMask( MSelectionMask::kSelectObjectsMask );
    MSelectionList item;
    item.add( selectInfo.selectPath() );
    MPoint xformedPt;
    selectInfo.addSelection( item, xformedPt, selectionList,
                             worldSpaceSelectPts, priorityMask, false );
    return true;
}

//////////////////////////////////////////////////////////////////
////  getPlugData is a util to update the drawing of the UI stuff

bool partioVisualizer::GetPlugData()
{
    MObject thisNode = thisMObject();
    int update = 0;
    MPlug updatePlug(thisNode, aUpdateCache );
    updatePlug.getValue( update );
    if (update != dUpdate)
    {
        dUpdate = update;
        return true;
    }
    else
        return false;
}

// note the "const" at the end, its different than other draw calls
void partioVisualizerUI::draw(const MDrawRequest& request, M3dView& view) const
{
    MDrawData data = request.drawData();

    partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();
    partioVizReaderCache* cache = (partioVizReaderCache*) data.geometry();

    MObject thisNode = shapeNode->thisMObject();
    MPlug sizePlug( thisNode, shapeNode->aSize );
    MDistance sizeVal;
    sizePlug.getValue( sizeVal );

    shapeNode->multiplier= (float) sizeVal.asCentimeters();

    int drawStyle = 0;
    MPlug drawStylePlug( thisNode, shapeNode->aDrawStyle );
    drawStylePlug.getValue( drawStyle );

    view.beginGL();

    if (drawStyle == 3 || view.displayStyle() == M3dView::kBoundingBox )
    {
        drawBoundingBox();
    }
    else
    {
        drawPartio(cache,drawStyle);
    }

	if (shapeNode->drawError == 1)
	{
		glColor3f(.75f,0.0f,0.0f);
	}
	else if (shapeNode->drawError == 2)
	{
		glColor3f(0.0f,0.0f,0.0f);
	}

	partio4Maya::drawPartioLogo(shapeNode->multiplier);

    view.endGL();
}

////////////////////////////////////////////
/// DRAW Bounding box
void  partioVisualizerUI::drawBoundingBox() const
{

    partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();

    MPoint  bboxMin = shapeNode->pvCache.bbox.min();
    MPoint  bboxMax = shapeNode->pvCache.bbox.max();

    float xMin = float(bboxMin.x);
    float yMin = float(bboxMin.y);
    float zMin = float(bboxMin.z);
    float xMax = float(bboxMax.x);
    float yMax = float(bboxMax.y);
    float zMax = float(bboxMax.z);

    /// draw the bounding box
    glBegin (GL_LINES);

    glColor3f(1.0f,0.5f,0.5f);

    glVertex3f (xMin,yMin,zMax);
    glVertex3f (xMax,yMin,zMax);

    glVertex3f (xMin,yMin,zMin);
    glVertex3f (xMax,yMin,zMin);

    glVertex3f (xMin,yMin,zMax);
    glVertex3f (xMin,yMin,zMin);

    glVertex3f (xMax,yMin,zMax);
    glVertex3f (xMax,yMin,zMin);

    glVertex3f (xMin,yMax,zMin);
    glVertex3f (xMin,yMax,zMax);

    glVertex3f (xMax,yMax,zMax);
    glVertex3f (xMax,yMax,zMin);

    glVertex3f (xMin,yMax,zMax);
    glVertex3f (xMax,yMax,zMax);

    glVertex3f (xMin,yMax,zMin);
    glVertex3f (xMax,yMax,zMin);


    glVertex3f (xMin,yMax,zMin);
    glVertex3f (xMin,yMin,zMin);

    glVertex3f (xMax,yMax,zMin);
    glVertex3f (xMax,yMin,zMin);

    glVertex3f (xMin,yMax,zMax);
    glVertex3f (xMin,yMin,zMax);

    glVertex3f (xMax,yMax,zMax);
    glVertex3f (xMax,yMin,zMax);

    glEnd();

}


////////////////////////////////////////////
/// DRAW PARTIO

void partioVisualizerUI::drawPartio(partioVizReaderCache* pvCache, int drawStyle) const
{
	partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();

    MObject thisNode = shapeNode->thisMObject();
    const int drawSkipVal = MPlug(thisNode, shapeNode->aDrawSkip).asInt();

    const bool flipYZVal = MPlug(thisNode, shapeNode->aFlipYZ).asBool();

    const int stride_position = 3 * (int)sizeof(float) * (drawSkipVal);
    const int stride_color = 4 * (int)sizeof(float) * (drawSkipVal);

    const float pointSizeVal = MPlug(thisNode, shapeNode->aPointSize).asFloat();
    const int alphaFromVal = MPlug(thisNode, shapeNode->aAlphaFrom).asInt();
    const float defaultAlphaVal = MPlug(thisNode, shapeNode->aDefaultAlpha).asFloat();

    // these two are not used
    // const int colorFromVal = MPlug(thisNode, shapeNode->aColorFrom).asInt();
    // const int incandFromVal = MPlug(thisNode, shapeNode->aIncandFrom).asInt();

    if (pvCache->particles)
    {
        const bool use_per_particle_alpha = alphaFromVal >=0 || defaultAlphaVal < 1;
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        if (use_per_particle_alpha) //testing settings
        {
            // TESTING AROUND with depth/transparency  sorting issues..
            glDepthMask(true);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glEnable(GL_POINT_SMOOTH);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        if (drawStyle == 0)
        {
			glDisable(GL_POINT_SMOOTH);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            glPointSize(pointSizeVal);
            if (pvCache->particles->numParticles() > 0)
            {
                // now setup the position/color/alpha output pointers

                const float * partioPositions = pvCache->particles->data<float>(pvCache->positionAttr,0);

                glVertexPointer(3, GL_FLOAT, stride_position, partioPositions);
                glColorPointer(4, GL_FLOAT, stride_color, pvCache->rgba.data());
                glDrawArrays( GL_POINTS, 0, (pvCache->particles->numParticles()/(drawSkipVal+1)) );
            }
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
        }
        else
        {
            /// looping thru particles one by one...
            glPointSize(pointSizeVal);

            const bool draw_billboards = drawStyle == 1 || drawStyle == 2;

            for (int i = 0; i < pvCache->particles->numParticles(); i += (drawSkipVal + 1))
            {
                if (use_per_particle_alpha)  // use transparency switch
                    glColor4f(pvCache->rgba[i * 4], pvCache->rgba[(i * 4) + 1], pvCache->rgba[(i * 4) + 2], pvCache->rgba[(i * 4) + 3]);
                else
                    glColor3f(pvCache->rgba[i * 4], pvCache->rgba[i * 4 + 1], pvCache->rgba[i * 4 + 2]);

                const float * partioPositions = pvCache->particles->data<float>(pvCache->positionAttr,i);
                if (draw_billboards) // unfilled circles, disks, or spheres
                    drawBillboardCircleAtPoint(partioPositions, pvCache->radius[i], 10, drawStyle);
                else // points
                    glVertex3f(partioPositions[0], partioPositions[1], partioPositions[2]);
            }
		}
        glDisable(GL_BLEND);
        glDisable(GL_POINT_SMOOTH);
        glPopAttrib();
    }
}

partioVisualizerUI::partioVisualizerUI()
{
}

partioVisualizerUI::~partioVisualizerUI()
{
}

void* partioVisualizerUI::creator()
{
    return new partioVisualizerUI();
}

void partioVisualizerUI::getDrawRequests(const MDrawInfo & info,
        bool /*objectAndActiveOnly*/, MDrawRequestQueue & queue)
{

    MDrawData data;
    MDrawRequest request = info.getPrototype(*this);
    partioVisualizer* shapeNode = (partioVisualizer*) surfaceShape();
    partioVizReaderCache* geom = shapeNode->updateParticleCache();

    getDrawData(geom, data);
    request.setDrawData(data);

    // Are we displaying dynamics?
    if (!info.objectDisplayStatus(M3dView::kDisplayDynamics))
        return;

    M3dView::DisplayStatus displayStatus = info.displayStatus();
    M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
    M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;
    switch (displayStatus)
    {
    case M3dView::kLead:
        request.setColor(LEAD_COLOR, activeColorTable);
        break;
    case M3dView::kActive:
        request.setColor(ACTIVE_COLOR, activeColorTable);
        break;
    case M3dView::kActiveAffected:
        request.setColor(ACTIVE_AFFECTED_COLOR, activeColorTable);
        break;
    case M3dView::kDormant:
        request.setColor(DORMANT_COLOR, dormantColorTable);
        break;
    case M3dView::kHilite:
        request.setColor(HILITE_COLOR, activeColorTable);
        break;
    default:
        break;
    }
    request.setIsTransparent( true );
    queue.add(request);
}
