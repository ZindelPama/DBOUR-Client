//**********************************************************************
//
// Copyright (c) 2002-2006
// PathEngine
// Lyon, France
//
// All Rights Reserved
//
//**********************************************************************

#include "platform_common/Header.h"
#include "cTestBed.h"
#include "MeshRenderInfo.h"
#include "PickPointOnMesh.h"
#include "GraphicsInterfaceRecorder.h"
#include "DrawShapes.h"
#include "TestBedUserData.h"
#include "interface/Singletons_TestBed.h"
#include "libs/TestBed_Application/interface/iOSInterface.h"
#include "libs/TestBed_Application/interface/iControlInterface.h"
#include "libs/TestBed_Application/interface/iGraphicsInterface.h"
#include "libs/TestBed_Application/interface/PlaySounds.h"
#include "libs/PathEngine_Interface/interface/cPathEngine.h"
#include "libs/PathEngine_Interface/interface/cAgent.h"
#include "libs/PathEngine_Core/interface/OverlappingPolygon.h"
#include "libs/PathEngine_Core/interface/Version.h"
#include "libs/Mesh2D/interface/CircuitElement.h"
#include "libs/Mesh3D/interface/tMesh_3D.h"
#include "libs/Geometry/interface/Point_PointLike.h"
#include "platform_common/PointMultiplication.h"
#include "common/Containers/SwapOut.h"
#include "common/MemoryTracking_Interface.h"


class cAgentRenderPreprocess
{
    cPosition _position;
    long _height;
    float _precisionX, _precisionY;
    long _countDownToRecording;
    cGraphicsInterfaceRecorder* _recorder;

public:

    cAgentRenderPreprocess()
    {
        _countDownToRecording = 4;
        _recorder = 0;
    }
    ~cAgentRenderPreprocess()
    {
        delete _recorder;
    }

    void render(iAgent* agent, cPosition position, long height, float precisionX, float precisionY)
    {
        if(position != _position
        || height != _height
        || precisionX != _precisionX
        || precisionY != _precisionY)
        {
            _position = position;
            _height = height;
            _precisionX = precisionX;
            _precisionY = precisionY;
            _countDownToRecording = 4;
        }
        if(_countDownToRecording > 1)
        {
            DrawAgentWithPrecision(nSingletons::testBed().getGIP(),
                        position,
                        nSingletons::testBed().refMesh().heightAtPosition(position),
                        agent->getShape(),
                        height,
                        precisionX, precisionY);
            --_countDownToRecording;
            return;
        }
        if(_countDownToRecording == 1)
        {
            delete _recorder;
            _recorder = new cGraphicsInterfaceRecorder(nSingletons::testBed().getGIP());
            DrawAgentWithPrecision(_recorder,
                        position,
                        nSingletons::testBed().refMesh().heightAtPosition(position),
                        agent->getShape(),
                        height,
                        precisionX, precisionY);
            _countDownToRecording = 0;
        }
        _recorder->play();
    }
};

cTestBedUserData&
cTestBed::meshUserData()
{
    return *static_cast<cTestBedUserData*>(_mesh->getTestBedUserData());
}

cTestBed::cTestBed(osinterface *ip) :
 cip(ip->getcontrolinterface()),
 gip(ip->getiGraphicsInterface()),
 ip(ip)
{
    _cameraPositionSet = false;

    _mesh=0;
    _pickedThisFrame=false;
    _previousMouseValid=false;
#ifdef PROFILING_ON
    _profile=new cProfile;
#endif
    DoRDTSC(_startHigh,_startLow);
    {
        int size=sizeof(_clocksBuffer)/sizeof(*_clocksBuffer);
        int i;
        for(i=0;i<size;i++)
            _clocksBuffer[i]=0.0;
    }
    _clocksBufferPos=0;

    _currentY = 0;
    _cachedBounds = new cGraphicsInterfaceRecorder(gip);
    _cachedBounds2 = new cGraphicsInterfaceRecorder(gip);
}

cTestBed::~cTestBed()
{
    releaseMesh();
#ifdef PROFILING_ON
    delete _profile;
#endif
    delete _cachedBounds;
    delete _cachedBounds2;
}

const char *const*
cTestBed::getVersionAttributes() const
{
    static bool firsttime = true;
    static const char *const* attributes;
    if(firsttime)
    {
        static cAttributesBuilder builder;
        firsttime=false;
        builder.add("buildstring", VersionTimeString());
        builder.add("release", MajorRelease());
        builder.add("minor_release", MinorRelease());
        builder.add("interface_major_version", TESTBED_INTERFACE_MAJOR_VERSION);
        builder.add("interface_minor_version", TESTBED_INTERFACE_MINOR_VERSION);
        if(MemoryTrackingIsEnabled())
        {
            builder.add("memory_tracking_enabled", "true");
        }
        attributes = builder.get();
    }
    return attributes;
}

void cTestBed::setMesh(iMesh *mesh)
{
    if(mesh == 0)
    {
        return;
    }
    if(_mesh)
    {
        releaseMesh();
    }
    _mesh = static_cast<cMesh*>(mesh);
    if(_mesh->getTestBedUserData() == 0)
    {
        _mesh->setTestBedUserData(new cTestBedUserData);
        meshUserData()._meshRenderInfo = new cMeshRenderInfo;
        meshUserData()._meshRenderInfo->build(_mesh->ref3D(),gip);
        GenerateMeshLOSPreprocess(_mesh->ref3D(), meshUserData()._losPreprocess);
    }
    _pickedThisFrame = false;
    _cachedBoundsValid = false;
    _cachedBounds2Valid = false;
    _cachedBoundsToggle = false;
}
void cTestBed::releaseMesh()
{
    if(_mesh == 0)
    {
        return;
    }
    _expansionRecorders.deleteAll();
    _smallConvexRecorders.deleteAll();
    _agentRenderPreprocess.deleteAll();

    _mesh = 0;
}

void cTestBed::zoomExtents()
{
    if(_mesh == 0)
    {
        return;
    }
    ZoomExtents(_mesh->ref3D(), ip->getwinx(), ip->getwiny());
    gip->setCamera(g_target_position.data, g_camera_angle1, g_camera_angle2, g_camera_distance);
    _cameraPositionSet = true;
}

const char* cTestBed::receiveKeyMessage()
{
    const char* message=cip->getkeymessage();
//    if(message && message[0]=='d' && message[1]=='P'
//        && getKeyState("_CONTROL") && getKeyState("_LSHIFT"))
//    {
//        PROFILER_START(100000);
//    }
//    if(message && message[0]=='d' && message[1]=='E'
//        && getKeyState("_CONTROL") && getKeyState("_LSHIFT"))
//    {
//        PROFILER_STOP;
//#ifdef PROFILING_ON
//        cProfileBuffer::m_theinstance.build(*_profile);
//#endif
//    }
    return message;
}
bool cTestBed::getKeyState(const char *keystring)
{
    return cip->getkeystate(keystring)!=0;
}
bool cTestBed::getLeftMouseState()
{
    return cip->getlmbdown()!=0;
}
bool cTestBed::getRightMouseState()
{
    return cip->getrmbdown()!=0;
}

void cTestBed::getMouseScreenPosition(long &screenx, long &screeny)
{
    screenx=cip->getmousex();
    screeny=cip->getmousey();
}
void cTestBed::getMouseScreenDelta(long &screendx, long &screendy)
{
    if(cip->getversion(1,2)==1)
    {
        if(_previousMouseValid)
        {
            screendx=cip->getmousex()-_previousMouseX;
            screendy=cip->getmousey()-_previousMouseY;
        }
        else
        {
            screendx=0;
            screendy=0;
        }
    }
    else
    {
        screendx=cip->getMouseDX();
        screendy=cip->getMouseDY();
    }
    _previousMouseX=cip->getmousex();
    _previousMouseY=cip->getmousey();
    _previousMouseValid=true;
}



cPosition cTestBed::positionAtMouse()
{
    if(!_mesh)
        return cPosition();

    if(_pickedThisFrame)
        return _pickPosition;

    cPosition pick_position;

    _pickScreenX=cip->getmousex();
    _pickScreenY=cip->getmousey();

    bool picked = PickPointOnMesh<tMesh_3D>(
                meshUserData()._losPreprocess,
                _mesh->ref3D(),
                _mesh->refTranslation(),
                GetCameraPosition(),
                ReverseProject(_pickScreenX, _pickScreenY),
                pick_position
                );

    if(!picked)
    {
        pick_position.cell=-1;
    }

    _pickedThisFrame=true;
    _pickPosition = pick_position;

    return pick_position;
}

void cTestBed::setColour(const char *colour)
{
    gip->setColour(colour);
}
void cTestBed::setColourRGB(float r, float g, float b)
{
    gip->setColour_RGB(r,g,b);
}



void cTestBed::drawMesh()
{
    if(_mesh)
    {
        gip->setZBias(0.0f);
        long i;
        for(i = 0; i < _mesh->ref3D().faces(); i++)
        {
            meshUserData()._meshRenderInfo->drawFace(i);
        }
    }
}

void
cTestBed::drawMeshEdges()
{
    if(_mesh)
    {
        gip->setZBias(-0.2f);
        tFace_3D f=_mesh->ref3D().beginFace();
        do
        {
            if(!f->isExternal())
                continue;
            tEdge_3D e=tMesh_3D::beginEdge(f);
            do
            {
                meshUserData()._meshRenderInfo->drawEdge(e.twin().index());
            }
            while(e.toNextInFace());
        }
        while((++f).valid());
    }
}
void
cTestBed::draw3DMeshInternalEdges()
{
    if(_mesh)
    {
        gip->setZBias(-0.2f);

//        const std::vector<long>* terrainLayers = _mesh->getTerrainLayersVector();

        tFace_3D f=_mesh->ref3D().beginFace();
        do
        {
            if(f->isExternal())
            {
                continue;
            }
//            if(terrainLayers && (*terrainLayers)[f.index()] != -1)
//            {
//                continue;
//            }
            tEdge_3D e=tMesh_3D::beginEdge(f);
            do
            {
                if(e.twin().face()->isExternal())
                {
                    continue;
                }
                meshUserData()._meshRenderInfo->drawEdge(e.index());
            }
            while(e.toNextInFace());
        }
        while((++f).valid());
    }
}


void cTestBed::drawMeshExpansion(iShape *shape)
{
    if(!_mesh)
        return;
    if(!shape)
        return;
    if(!_mesh->shapeCanCollide(shape))
        return;
    cShape *cs=static_cast<cShape*>(shape);
    gip->setZBias(-0.2f);
    long index = cs->getPreprocessIndex();
    if(!_expansionRecorders[index])
    {
        _expansionRecorders.set(index, new cGraphicsInterfaceRecorder(gip));
        const tMeshCutSequence& cuts = _mesh->refEdgeExpansion(cs);
        DrawMeshCutSequence(cuts, _expansionRecorders[index], _mesh->refTranslation());
        const cCollisionPreprocess& cp = _mesh->refCollisionPreprocess(index);
        if(_mesh->shapeCanPathfind(shape))
        {
            const cPathfindPreprocess& pp = _mesh->refPathfindPreprocess(index);
            if(pp.smallConvexWereSplit())
            {
                DrawShapes(
                        *pp.getNonSmallConvex(),
                        _expansionRecorders[index], _mesh->refTranslation()
                        );
            }
            else
            {
                DrawShapes(
                        cp.refBaseObstacleExpansions(),
                        _expansionRecorders[index], _mesh->refTranslation()
                        );
            }
        }
        else
        {
            DrawShapes(
                    cp.refBaseObstacleExpansions(),
                    _expansionRecorders[index], _mesh->refTranslation()
                    );
        }
    }
    _expansionRecorders[index]->play();
}
void cTestBed::drawAgent(iAgent* agent, long height)
{
    drawAgentWithPrecision(agent, height, 0.f, 0.f);
}
void cTestBed::drawShape(iShape *shape, const cPosition &position, long height)
{
    if(!_mesh)
        return;
    if(!shape)
        return;
    if(position.cell<0)
        return;
    gip->setZBias(0.0f);
    DrawAgent(    gip,
                position,
                _mesh->heightAtPosition(position),
                shape,
                height);
}
void
cTestBed::drawAgentHeading(iAgent* agent, long size, long height, float heading)
{
    if(!_mesh)
        return;
    if(!agent)
        return;
    cPosition position=agent->getPosition();
    if(position.cell<0)
        return;
    gip->setZBias(-0.45f);
    DrawAgentArrow(gip, position, heading, _mesh->heightAtPosition(position), static_cast<float>(size), height);
}
void cTestBed::drawPath(iPath* path)
{
    if(!path)
    {
        return;
    }
    long points = path->size();
    if(points == 0)
    {
        return;
    }
    const cPosition* pointArray = path->getPositionArray();
    long i;
    gip->setZBias(-0.2f);
    for(i = 0; i < points - 1; i++)
    {
        cInternalPosition start, end;
        _mesh->convertPosition(pointArray[i], start);
        _mesh->convertPosition(pointArray[i + 1], end);
        tLine l(start.p, end.p);
        tFace f = FaceAtEndOfLine<tMesh>(start.f, l);
        if(f == end.f)
        {
            drawLine(pointArray[i], pointArray[i + 1]);
        }
        else
        {
            float startHeight = _mesh->heightAtPositionF(pointArray[i]);
            float endHeight = _mesh->heightAtPositionF(pointArray[i + 1]);
            cVector3F start3F(static_cast<float>(start.p.getX()), static_cast<float>(start.p.getY()), startHeight);
            cVector3F end3F(static_cast<float>(end.p.getX()), static_cast<float>(end.p.getY()), endHeight);
            Draw3DLine(gip, start3F, end3F);
        }
    }
}
void cTestBed::drawPosition(const cPosition &position, long size)
{
    if(!_mesh)
        return;
    if(position.cell<0)
        return;        
    gip->setZBias(-0.45f);

    Draw3DCross(
            gip,
            cVector3F(static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(_mesh->heightAtPosition(position))),
            static_cast<float>(size)
            );
}
void cTestBed::drawLine(const cPosition &start, const cPosition &end)
{
    if(!_mesh)
        return;
    if(start.cell<0)
        return;        
    tPoint startP = tPoint(start.x, start.y, WORLD_RANGE);
    tPoint endP = tPoint(end.x, end.y, WORLD_RANGE);

    if(startP == endP)
    {
        return;
    }

    cInternalPosition startIP;
    _mesh->convertPosition(start, startIP);

    tFace_3D f3D = refMesh().refTranslation().translate(startIP.f, startIP.p);

    gip->setZBias(-0.35f);
    DrawLineOnGround(nSingletons::testBed().getGIP(), f3D, tLine(startP, endP), SIDE_RIGHT);
}




void
cTestBed::drawSquareBounds(const cPosition &start, long distance)
{
    if(!_mesh)
    {
        return;
    }
    if(start.cell < 0)
    {
        return;        
    }
    cInternalPosition start_ip;
    _mesh->convertPosition(start, start_ip);

    std::list<tFace> contained_faces;
    cHandle<tMeshCircuits> bounds = new tMeshCircuits;
    GetSquareBounds(start_ip, distance, contained_faces, *bounds);

    gip->setZBias(-0.35f);
    DrawMeshCircuits(*bounds, gip, _mesh->refTranslation());
}

void
cTestBed::drawRangeBounds(long minX, long minY, long maxX, long maxY)
{
    if(_cachedBoundsValid
     && _cachedBoundsMinX == minX
     && _cachedBoundsMinY == minY
     && _cachedBoundsMaxX == maxX
     && _cachedBoundsMaxY == maxY
     )
    {
        _cachedBounds->play();
        return;
    }
    if(_cachedBounds2Valid
     && _cachedBounds2MinX == minX
     && _cachedBounds2MinY == minY
     && _cachedBounds2MaxX == maxX
     && _cachedBounds2MaxY == maxY
     )
    {
        _cachedBounds2->play();
        return;
    }
    if(maxX <= minX || maxY <= minY)
    {
        Error("NonFatal", "cTestBed::drawRangeBounds(): bad bounds.");
        return;
    }
    if(!_mesh)
    {
        return;
    }
    long x = maxX - minX;
    if(x < 2)
    {
        Error("NonFatal", "cTestBed::drawRangeBounds(): maxX - minX must be >= 2.");
        return; // pathological case where we can't make a poly that contains it's origin
    }
    if(x > SHAPE_RANGE * 2)
    {
        Error("NonFatal", "cTestBed::drawRangeBounds(): maxX - minX is greater than allowed range.");
        return; // outside range declared below
    }
    x /= 2;
    x += minX;
    long y = maxY - minY;
    if(y < 2)
    {
        Error("NonFatal", "cTestBed::drawRangeBounds(): maxY - minY must be >= 2.");
        return; // pathological case where we can't make a poly that contains it's origin
    }
    if(y > SHAPE_RANGE * 2)
    {
        Error("NonFatal", "cTestBed::drawRangeBounds(): maxY - minY is greater than allowed range.");
        return; // outside range declared below
    }
    y /= 2;
    y += minY;
    tPoint centre(x, y, WORLD_RANGE);
    tPoint pointArray[4];
    pointArray[0] = tPoint(minX - x, minY - y, SHAPE_RANGE);
    pointArray[1] = tPoint(minX - x, maxY - y, SHAPE_RANGE);
    pointArray[2] = tPoint(maxX - x, maxY - y, SHAPE_RANGE);
    pointArray[3] = tPoint(maxX - x, minY - y, SHAPE_RANGE);
    cCollisionShape shape(4, pointArray);
    std::list<tFace> containedFaces;
    cHandle<tMeshCircuits> bounds = CircuitsForInfiniteCylinder(
            _mesh->ref2D(),
            centre,
            shape,
            containedFaces
            );

    gip->setZBias(-0.35f);
//    DrawMeshCircuits(*bounds, gip, _mesh->refTranslation());
    if(!_cachedBoundsValid || !_cachedBoundsToggle)
    {
        _cachedBoundsValid = true;
        _cachedBoundsMinX = minX;
        _cachedBoundsMinY = minY;
        _cachedBoundsMaxX = maxX;
        _cachedBoundsMaxY = maxY;
        _cachedBounds->clear();
        DrawMeshCircuits(*bounds, _cachedBounds, _mesh->refTranslation());
        _cachedBounds->play();
        _cachedBoundsToggle = true;
    }
    else
    {
        _cachedBounds2Valid = true;
        _cachedBounds2MinX = minX;
        _cachedBounds2MinY = minY;
        _cachedBounds2MaxX = maxX;
        _cachedBounds2MaxY = maxY;
        _cachedBounds2->clear();
        DrawMeshCircuits(*bounds, _cachedBounds2, _mesh->refTranslation());
        _cachedBounds2->play();
        _cachedBoundsToggle = false;
    }
}


void cTestBed::drawAgentExpansion(iAgent* agent, iShape *expandedby)
{
    if(!_mesh)
        return;
    if(!agent || !expandedby)
        return;
    if(!_mesh->shapeCanCollide(expandedby))
        return;
    gip->setZBias(-0.35f);
    cAgent *c_agent=static_cast<cAgent*>(agent);
//    {
//        const cAgentPreprocess &preprocess=c_agent->refAgentPreprocess();
//        const std::list<tFace> &list=preprocess.refContainedExternalFaces();
//        std::list<tFace>::const_iterator i;
//        for(i=list.begin();i!=list.end();++i)
//        {
//            tFace_3D f;
//            _mesh->getFaceOnOverlapped(*i,f);
//            tEdge_3D e=tMesh_3D::beginEdge(f);
//            do
//            {
//                _meshRenderInfo->drawEdge(e.index());
//            }
//            while(e.toNextInFace());
//        }
//    }
    cShape *c_expandedby=static_cast<cShape*>(expandedby);
    {
        cHandle<cShapeRenderPreprocess> rp = c_agent->getRenderPreprocess(c_expandedby->getPreprocessIndex());
        cQueryContext& qc = _mesh->enterQuery();
        DrawMeshCircuits(rp->refCircuits(), gip, _mesh->refTranslation());
        _mesh->exitQuery(qc);
    }

}


void cTestBed::printText8by16(long x, long y, const char *text)
{
    iRenderGeometry* rg = gip->newText_8By16(x, y, text);
    gip->render_ScreenSpace(rg);
}
void cTestBed::printTextLine(long x, const char *text)
{
    iRenderGeometry* rg = gip->newText_8By16(x, _currentY, text);
    gip->render_ScreenSpace(rg);
    _currentY += 16;
}

void cTestBed::update()
{
    PROFILER_SCOPE("TESTBEDUPDATE");
    {
        unsigned long end_high,end_low;
        double start,end;
        DoRDTSC(end_high,end_low);
        start=_startHigh;
        start*=4294967296.0;
        start+=_startLow;
        end=end_high;
        end*=4294967296.0;
        end+=end_low;
        end-=start;        

        int size=sizeof(_clocksBuffer)/sizeof(*_clocksBuffer);

        _clocksBuffer[_clocksBufferPos++]=end;
        if(_clocksBufferPos==size)
            _clocksBufferPos=0;

        double total=0;
        {
            int i;
            for(i=0;i<size;i++)
                total+=_clocksBuffer[i];
        }

//            char buffer[100];
//            sprintf(buffer,"average clocks=%011.0f",total/size);
//            printText8by16(10,10,buffer);
    }
    if(ShouldCameraTargetBeDisplayed())
    {
        setColour("white");
        Draw3DCross(gip,g_target_position,20.0f);
    }
    if(!_cameraPositionSet)
    {
        CameraControl(cip, ip);
        gip->setCamera(g_target_position.data, g_camera_angle1, g_camera_angle2, g_camera_distance);
    }

    if(ip->isMinimised())
    {
        ip->update_DontBeginScene();
        while(ip->isMinimised())
        {
            ip->sleep(400);
            ip->update_Redraw();
//            ip->update_NoDraw();
        }
        ip->beginScene();
    }

    ip->update();
    _cameraPositionSet = false;
    _pickedThisFrame=false;

    _currentY = 0;

    DoRDTSC(_startHigh,_startLow);
}




void
cTestBed::drawIntegerCrossesAround(const cPosition& position, long radius, long cross_size)
{
    invalid();
}

void cTestBed::draw3DFace(tFace_3D f)
{
    if(_mesh)
    {
        meshUserData()._meshRenderInfo->drawFace(f.index());
    }
}
void cTestBed::draw3DEdge(tEdge_3D e)
{
    if(_mesh)
    {
        meshUserData()._meshRenderInfo->drawEdge(e.index());
    }
}

void
cTestBed::drawBurntInObstacles()
{
    //..... draw base obstacles directly instead
    if(!_mesh)
    {
        return;
    }
    if(!meshUserData()._baseCircuitsRecorder)
    {
        cHandle<tMeshCircuits> baseCircuits = _mesh->generateOldStyleBaseCircuitsForBaseObstacles();
        meshUserData()._baseCircuitsRecorder = new cGraphicsInterfaceRecorder(gip);
        DrawMeshCircuits(*baseCircuits, meshUserData()._baseCircuitsRecorder, _mesh->refTranslation());
    }
    meshUserData()._baseCircuitsRecorder->play();
}

void
cTestBed::drawSmallConvexExpansions(iShape *shape)
{
    if(!_mesh)
        return;
    if(!shape)
        return;
    if(!_mesh->shapeCanPathfind(shape))
        return;
    cShape *cs=static_cast<cShape*>(shape);
    gip->setZBias(-0.2f);
    long index = cs->getPreprocessIndex();
    if(!_smallConvexRecorders[index])
    {
        _smallConvexRecorders.set(index, new cGraphicsInterfaceRecorder(gip));
        const cPathfindPreprocess& pp = _mesh->refPathfindPreprocess(index);
        if(pp.smallConvexWereSplit())
        {
            DrawShapes(
                    *pp.getSmallConvexOnOriginal(),
                    _smallConvexRecorders[index], _mesh->refTranslation()
                    );
        }
//        const tMeshCircuits& circuits = _mesh->refSmallConvexExpansion(cs);
//        DrawMeshCircuits(circuits, _smallConvexRecorders[index], _mesh->refTranslation());
    }
    _smallConvexRecorders[index]->play();
}

void
cTestBed::drawAgentsInContext(iCollisionContext* context, long height)
{
    if(!context)
    {
        return;
    }
    long i;
    for(i = 0; i < context->getNumberOfAgents(); i++)
    {
        drawAgent(context->getAgent(i), height);
    }
}

void
cTestBed::lookAt(const cPosition& target, float angle, float elevation, float distance)
{
    if(!_mesh)
    {
        return;
    }
    cVector3F target3D(
                static_cast<float>(target.x),
                static_cast<float>(target.y), 
                static_cast<float>(_mesh->heightAtPosition(target))
                );
    LookAt(target3D, angle, elevation, distance);
    gip->setCamera(g_target_position.data, g_camera_angle1, g_camera_angle2, g_camera_distance);
    _cameraPositionSet = true;
}

bool
cTestBed::meshObstructsCamera()
{
    if(!_mesh)
    {
        return false;
    }
    if(g_camera_distance < 30.0f)
    {
        return false;
    }
    cVector3F start = GetCameraPosition();
    cVector3F toTarget = g_target_position - start;
    float fractionAlong = (g_camera_distance - 30.0f) / g_camera_distance;
    cVector3F end = start + toTarget * fractionAlong;
    nLineCollidesWithTriangle::cLinePreprocess line(start, end);
    long faceIndex;
    return LineCollidesWithMesh(line, meshUserData()._losPreprocess, faceIndex);
}
void
cTestBed::setAdditiveBlending(float alpha)
{
    gip->setAdditiveBlending(alpha);
}
void
cTestBed::setOpaque()
{
    gip->setOpaque();
}

void
cTestBed::drawAgentWithPrecision(iAgent* agent, long height, float precisionX, float precisionY)
{
    if(!_mesh)
        return;
    if(!agent)
        return;
    cPosition position = agent->getPosition();
    if(position.cell < 0)
        return;
    gip->setZBias(0.0f);

    long index = FromI(agent)->getIndexInMesh();

    if(!_agentRenderPreprocess[index])
    {
        _agentRenderPreprocess.set(index, new cAgentRenderPreprocess);
    }
    _agentRenderPreprocess[index]->render(agent, position, height, precisionX, precisionY);
}
void
cTestBed::drawAgentHeadingWithPrecision(iAgent* agent, long size, long height, float heading, float precisionX, float precisionY)
{
    if(!_mesh)
        return;
    if(!agent)
        return;
    cPosition position=agent->getPosition();
    if(position.cell<0)
        return;
    gip->setZBias(-0.45f);
    DrawAgentArrowWithPrecision(
            gip, position, heading,
            _mesh->heightAtPosition(position),
            static_cast<float>(size), height,
            precisionX, precisionY
            );
}

void
cTestBed::lookAtWithPrecision(const cPosition& target, float precisionX, float precisionY, float angle, float elevation, float distance)
{
    if(!_mesh)
    {
        return;
    }
    cVector3F target3D(
            static_cast<float>(target.x) + precisionX,
            static_cast<float>(target.y) + precisionY, 
            static_cast<float>(_mesh->heightAtPosition(target))
            );
    LookAt(target3D, angle, elevation, distance);
    gip->setCamera(g_target_position.data, g_camera_angle1, g_camera_angle2, g_camera_distance);
    _cameraPositionSet = true;
}

long
cTestBed::createSound(const char* fileName, long numberOfBuffers)
{
    return CreateSound(fileName, numberOfBuffers);
}
void
cTestBed::playSound(long index)
{
    if(index >= 0)
    {
        PlaySoundZ(index);
    }
}

long
cTestBed::loadSplashImage(const char* fileName)
{
    return gip->loadImage(fileName);
}
void
cTestBed::displaySplashImage(long x, long y, long imageIndex)
{
    iRenderGeometry* rg = gip->newSplashImage(x, y, imageIndex);
    gip->render_ScreenSpace(rg);
}
void
cTestBed::displaySplashImage_Centred(long imageIndex)
{
    iRenderGeometry* rg = gip->newSplashImage(imageIndex);
    gip->render_ScreenSpace(rg);
}

void
cTestBed::update_DontBeginScene()
{
    if(ShouldCameraTargetBeDisplayed())
    {
        setColour("white");
        Draw3DCross(gip,g_target_position,20.0f);
    }
    if(!_cameraPositionSet)
    {
        CameraControl(cip, ip);
        gip->setCamera(g_target_position.data, g_camera_angle1, g_camera_angle2, g_camera_distance);
    }
    ip->update_DontBeginScene();
    _cameraPositionSet = false;
    _pickedThisFrame=false;
    _currentY = 0;
}
void
cTestBed::update_Redraw()
{
    ip->update_Redraw();
//    _cameraPositionSet = false;
//    _pickedThisFrame=false;
}
void
cTestBed::beginScene()
{
    ip->beginScene();
}


//void
//cTestBed::supplyTerrainHeightField(long terrainLayer,
//                                  const float* heights,
//                                  long startX, long startY,
//                                  long gridSpacing, long gridWidth, long gridHeight)
//{
//    if(!_mesh)
//    {
//        return;
//    }
//    if(terrainLayer < 0)
//    {
//        return;
//    }
//    if(terrainLayer >= _suppliedHeightFields.size())
//    {
//        _suppliedHeightFields.resize(terrainLayer + 1, 0);
//    }
//    if(_suppliedHeightFields[terrainLayer])
//    {
//        return;
//    }
//    _suppliedHeightFields[terrainLayer] = new cSuppliedHeightField(heights, startX, startY, gridSpacing, gridWidth, gridHeight);
//    _mesh->setTerrainCallBack(terrainLayer, _suppliedHeightFields[terrainLayer]);
//}
