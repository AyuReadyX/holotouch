#include "glwidget.h"

#include <math.h>
#include <GL/glu.h>
#include <iostream>

#include <QMutexLocker>

#include "leapmotion/HandEvent.h"

#define DEFAULT_SPACING 2.0f

GlWidget::item_t::item_t(const QString& pName, float pSize, texId_t pText)
    :x_(0),
     y_(0),
     z_(0),
     size_(pSize),
     sizeOffset_(0),
     texture_(pText),
     selected_(false),
     drawn_(false),
     fileName_(pName)
{
}

GlWidget::GlWidget(QWidget *parent) :
    Glview(60,parent),
    selectionMode_(HandEvent::SINGLE),
    boxSize_(BOX_SIZE),
    gridSize_(0),
    spacing_(DEFAULT_SPACING),
    fileExplorer_(QDir::home()),
    currentAnim_(IDLE)
{
    leapListener_.setReceiver(this);
    controller_.addListener(leapListener_);
    head_.x = 0.0;
    head_.y = 0.0;
    head_.z = 5.0;
    palmPos_.x = 0.0f;
    palmPos_.y = 0.0f;
    palmPos_.z = 5.0f;
    setCursor(Qt::BlankCursor);
    //generateCubes(CRATE,5*5*5);
    reloadFolder();
}

GlWidget::~GlWidget()
{
    controller_.removeListener(leapListener_);
}

void GlWidget::initializeGL()
{
    loadTexture("../code/ressources/box.png", CRATE);
    loadTexture("../code/ressources/metal.jpg", METAL);
    loadTexture("../code/ressources/Folder.png", FOLDER);
    loadTexture("../code/ressources/music.png", MUSIC);
    loadTexture("../code/ressources/picture.png", PICTURE);
    loadTexture("../code/ressources/text.png", TEXT);
    loadTexture("../code/ressources/video.png", VIDEO);

    glEnable(GL_TEXTURE_2D);

    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void GlWidget::resizeGL(int width, int height)
{
    if(height == 0)
        height = 1;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)width/(GLfloat)height, 1.0f, -100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void GlWidget::paintGL()
{
    // ============================
    // Render Scene
    // ============================

    // clear the back buffer and z buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // disable lighting
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

    gluLookAt(head_.x,head_.y,head_.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,0.0f);

    // Objects
    drawPalmPos();
    computeGrid(boxSize_);
    handleSelection();
    drawCurrentGrid();
}

//helper function, loads a texture and assign it to an enum value
//to help retrieve it later
void GlWidget::loadTexture(QString textureName, texId_t pId)
{
    QImage qim_Texture;
    QImage qim_TempTexture;
    qim_TempTexture.load(textureName);
    qim_Texture = QGLWidget::convertToGLFormat( qim_TempTexture );
    glGenTextures( 1, &texture_[pId] );
    glBindTexture( GL_TEXTURE_2D, texture_[pId] );
    glTexImage2D( GL_TEXTURE_2D, 0, 3, qim_Texture.width(), qim_Texture.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, qim_Texture.bits() );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
}

//Draw 6 squares and apply the texture on each: absolute coordinates for the center
void GlWidget::drawCube(texId_t PtextureId, float pCenterX, float pCenterY,float pCenterZ, float pSize)
{
    float half = pSize/2;
    glBindTexture(GL_TEXTURE_2D, texture_[PtextureId]);

    glBegin(GL_QUADS);
    // front fixed Z near (positive)
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ-half);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ-half);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX+half, pCenterY+half, pCenterZ-half);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX-half, pCenterY+half, pCenterZ-half);

    // back fixed z far (negative)
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ+half);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX-half, pCenterY+half, pCenterZ+half);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX+half, pCenterY+half, pCenterZ+half);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ+half);

    // top fixed Y up
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX-half,  pCenterY+half, pCenterZ-half);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX-half,  pCenterY+half, pCenterZ+half);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX+half,  pCenterY+half, pCenterZ+half);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX+half,  pCenterY+half, pCenterZ-half);

    // bottom fixed Y down
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ-half);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ-half);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ+half);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ+half);

    // Right fixed X (positive)
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ-half);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX+half, pCenterY+half, pCenterZ-half);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX+half, pCenterY+half, pCenterZ+half);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ+half);

    // Left fixed x negative
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ-half);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ+half);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX-half, pCenterY+half, pCenterZ+half);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX-half, pCenterY+half, pCenterZ-half);
    glEnd();
}

//Draw a tile of pSize, apply texture everywhere (to be enhanced)
void GlWidget::drawTile(texId_t PtextureId, float pCenterX, float pCenterY,float pCenterZ, float pSize)
{
    float half = pSize/2;
    float thickness = pSize/8;
    glBindTexture(GL_TEXTURE_2D, texture_[PtextureId]);

    glBegin(GL_QUADS);
    // front fixed Z near (positive)
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ-thickness);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ-thickness);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX+half, pCenterY+half, pCenterZ-thickness);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX-half, pCenterY+half, pCenterZ-thickness);

    // back fixed z far (negative)
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ+thickness);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX-half, pCenterY+half, pCenterZ+thickness);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX+half, pCenterY+half, pCenterZ+thickness);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ+thickness);

    // top fixed Y up
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX-half,  pCenterY+half, pCenterZ-thickness);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX-half,  pCenterY+half, pCenterZ+thickness);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX+half,  pCenterY+half, pCenterZ+thickness);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX+half,  pCenterY+half, pCenterZ-thickness);

    // bottom fixed Y down
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ-thickness);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ-thickness);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ+thickness);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ+thickness);

    // Right fixed X (positive)
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ-thickness);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX+half, pCenterY+half, pCenterZ-thickness);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX+half, pCenterY+half, pCenterZ+thickness);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX+half, pCenterY-half, pCenterZ+thickness);

    // Left fixed x negative
    glTexCoord2f(0.0f, 0.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ-thickness);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(pCenterX-half, pCenterY-half, pCenterZ+thickness);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(pCenterX-half, pCenterY+half, pCenterZ+thickness);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(pCenterX-half, pCenterY+half, pCenterZ-thickness);
    glEnd();
}

void GlWidget::drawTile(const item_t& pItem)
{
    drawTile(pItem.texture_,
             pItem.x_,
             pItem.y_,
             pItem.z_,
             pItem.size_ + pItem.sizeOffset_);
    if (pItem.fileName_.size() > 0)
    {
        //text is masked by cubes,
        //draw under to see it clearly
        renderText(pItem.x_ - pItem.size_/2.0f,
                   pItem.y_ - pItem.size_,
                   pItem.z_ + pItem.size_/2.0f,
                   pItem.fileName_);
    }
}

//Draw a 2D grid composed of L*H cubes of size CubeZise spaced by pSpacing
void GlWidget::drawCube2DGrid(texId_t pTexture,float pSpacing, float pCubeSize, int pL,int pH)
{
    glTranslatef(-(pSpacing+pCubeSize)*(pL-1)/2,(pSpacing+pCubeSize)*(pH-1)/2,0);
    for(int i = 0; i < pH; ++i)
    {
        for(int j = 0; j < pL; ++j)
        {
            drawCube(pTexture,0,0,0,pCubeSize);
            //draw next cube to the right (greater X)
            glTranslatef(pSpacing+pCubeSize,0,0);
        }
        //go back at the start of the line and draw downards y
        glTranslatef(-(pSpacing + pCubeSize)*pL,-(pSpacing+pCubeSize),0);
    }
}

//Draw a 3D grid composed of L*H cubes of size CubeZise spaced by pSpacing
void GlWidget::drawCube3DGrid(texId_t pTexture,
                              float pSpacing,
                              float pCubeSize,
                              int pL,
                              int pH,
                              int pW)
{
    glTranslatef(-(pSpacing+pCubeSize)*(pL-1)/2,
                 (pSpacing+pCubeSize)*(pH-1)/2,
                 -(pSpacing+pCubeSize)*(pW-1)/2);
    for(int z = 0; z < pW; z++)
    {
        for(int y = 0; y < pH; ++y)
        {
            for(int x = 0; x < pL; ++x)
            {
                drawCube(pTexture,0,0,0,pCubeSize);
                //draw next cube to the right (greater X)
                glTranslatef(pSpacing+pCubeSize,0,0);
            }
            //go back at the start of the line and draw downards y
            glTranslatef(-(pSpacing + pCubeSize)*pL,-(pSpacing+pCubeSize),0);
        }
        glTranslatef(0,(pSpacing+pCubeSize)*pH,-(pSpacing+pCubeSize));
    }
}

//overloaded function for ease of use
void GlWidget::drawCube(const item_t& pCube)
{
    drawCube(pCube.texture_,
             pCube.x_,
             pCube.y_,
             pCube.z_,
             pCube.size_ + pCube.sizeOffset_);
    if (pCube.fileName_.size() > 0)
    {
        //text is masked by cubes,
        //draw under to see it clearly
        renderText(pCube.x_ - pCube.size_/2.0f,
                   pCube.y_ - pCube.size_,
                   pCube.z_ + pCube.size_/2.0f,
                   pCube.fileName_);
    }
}

//draw a cube where the middle of the palm is
void GlWidget::drawPalmPos()
{
    //normalize leap coordinates to our box size
   drawCube(METAL,
            palmPos_.x ,
            palmPos_.y,
            palmPos_.z, BOX_SIZE/(4*gridSize_));
}

//init the view with a certain amount of cubes
void GlWidget::generateCubes(texId_t pTexture, int pNbCubes)
{
    itemList_.clear();
    for (int i=0; i<pNbCubes; ++i)
    {
        item_t item("",spacing_/3.0f, pTexture);
        itemList_.append(item);
    }
}

/*generate a cubic grid from itemList_
 *the grid is always in a box of BOX_SIZE
 * so the more cubes, the more smaller it appears
 */
void GlWidget::computeGrid(float pBoxSize)
{
    int nbItem = std::roundf(std::cbrt(itemList_.size()));

    //number of cube per dimension
    gridSize_ = nbItem;

    //distance between 2 cube's centers
    spacing_ = pBoxSize/nbItem;

    //apparent size of the cube
    float size = spacing_/2;

    //to center front face on (0,0,0)
    float offset = (nbItem-1)*spacing_/2;
    QMutexLocker locker(&mutexList_);
    for (int z = 0; z <= nbItem; z++)
    {
        for (int y = 0; y <= nbItem; y++)
        {
            for (int x = 0; x <= nbItem; x++)
            {
                int i = z*nbItem*nbItem + y*nbItem + x;
                //avoid overflow
                if (i < itemList_.size() )
                {
                    itemList_[i].size_ = size;
                    itemList_[i].x_ = x*spacing_ - offset;
                    itemList_[i].y_ = y*spacing_ - offset;
                    itemList_[i].z_ = -z*spacing_;
                }
            }
        }
    }
}

//update the view, draw items with absolute center coordinates
void GlWidget::drawCurrentGrid()
{
    QList<item_t>::iterator it;
    for (it = itemList_.begin(); it != itemList_.end(); it++)
        drawTile(*it);
        //drawCube(*it);
}

//find the closest cube from the palm center
int GlWidget::closestItem(float pTreshold)
{
    float minDist = 1000.0f;
    QList<item_t>::iterator it;
    int id = -1, i = 0;
    for (it = itemList_.begin(); it != itemList_.end(); it++)
    {
        Leap::Vector testV(it->x_,it->y_, it->z_);
        float delta = palmPos_.distanceTo(testV);
        if ((delta <= pTreshold) && (delta <= minDist))
        {
            minDist = delta;
            id = i;
        }
        i++;
    }
    return id;
}

/*Detect if a cube needs to be selected
 *perform growing cube animation on each
 *selected cube
 */
void GlWidget::handleSelection()
{
    //growing animation on selected cubes
    for (QList<item_t>::iterator it = itemList_.begin(); it != itemList_.end(); it++)
      {
        if ( it->selected_ )
        {
             if(it->sizeOffset_ <= it->size_)
            {
                it->sizeOffset_ += it->size_/10;
            }
        }
        else if ( it->sizeOffset_ > 0)
        {
            it->sizeOffset_ -= it->size_/20;
            if ( it->sizeOffset_ <= 0 )
                it->sizeOffset_ = 0;
        }
    }
}

void GlWidget::changeDirectory(const QString& pFolder)
{
    bool ok = false;
    if ( pFolder == "..")
    {
        if( fileExplorer_.cdUp() )
            ok = true;
    }
    else if ( fileExplorer_.cd(pFolder) )
        ok = true;
    if ( ok )
        reloadFolder();
}

//generate the view items from the files in a folder
void GlWidget::reloadFolder()
{
    //protect access on the datalist
    QMutexLocker locker(&mutexList_);

    qDebug() << "loaded folder: "<< fileExplorer_.path();

    QFileInfoList fileList = fileExplorer_.entryInfoList();
    QFileInfoList::const_iterator it;
    QList<item_t> newList;

    for( it = fileList.cbegin(); it != fileList.cend(); it++)
    {
        //TODO: choose better textures
        texId_t texture = CRATE;
        if ( it->isDir() )
            texture = FOLDER;
        else
        {
            //TODO: add more extensions
            QString ext = it->suffix();
            if ( ext == "png" ||
                 ext == "jpg" ||
                 ext == "bmp")
                texture = PICTURE;
            else if ( ext == "mp3" ||
                      ext == "wav" ||
                      ext == "ogg" ||
                      ext == "flac")
                texture = MUSIC;
           else if ( ext == "txt" ||
                     ext == "sh" ||
                     ext == "cpp" ||
                     ext == "py" )
                texture = TEXT;
            else if ( ext == "mp4" ||
                      ext == "avi" ||
                      ext == "mkv")
                texture = VIDEO;
        }
        //create new cube, size doesn't matter, recomputed each time
        item_t item(it->fileName(), 1.0f, texture);
        newList.append(item);
    }
    itemList_ = newList;
}

void GlWidget::customEvent(QEvent* pEvent)
{
    HandEvent* event = dynamic_cast<HandEvent*>(pEvent);
    if ( event )
    {
        //always get the new pos of the hand
        palmPos_ = event->pos();

        //detect if hand is near an item
        int item = closestItem(spacing_);
        leapListener_.setItem(item);

        //handle type of event
        switch (event->type() )
        {
        case HandEvent::Opened:
            break;
        case HandEvent::Closed:
            break;
        case HandEvent::Clicked:
            item = event->item();
            selectionMode_ = event->selectMode();
            slotSelect(item);
            break;
        case HandEvent::DoubleClicked:
            break;
        default:
            break;
        }
    }
}

//update the camera position
void GlWidget::slotNewHead(head_t pPos)
{
    /*We inverse axes to compensate head position relative
     * to the cube.
     */
    head_.x = -pPos.x;
    head_.y = -pPos.y;
    head_.z =  pPos.z;
}

//move slightly the camera, via keyboard commands for example
void GlWidget::slotMoveHead(int pAxis, float pDelta)
{
    switch(pAxis)
    {
        case 0:
            head_.x += pDelta;
            break;
        case 1:
            head_.y += pDelta;
            break;
        case 2:
            head_.z += pDelta;
        default:
            break;
    }
}

//called when select gesture is made
void GlWidget::slotSelect(int pItem)
{
    //hand is on a single item
    if (pItem!= -1 )
    { 
        if ( selectionMode_ == HandEvent::SINGLE )
        {
            //select pItempreviously not selected
            if ( !itemList_[pItem].selected_ )
            {
                itemList_[pItem].selected_ = true;
                for( int i = 0; i < itemList_.size(); i++ )
                {
                    if (i != pItem)
                        itemList_[i].selected_ = false;
                }
            }
            else //open previously selected item
            {
                if ( pItem< fileExplorer_.entryInfoList().size() )
                {
                    QFileInfo info =  fileExplorer_.entryInfoList().at(pItem);
                    if ( info.isDir() )
                    {
                        //reset view to new folder
                        changeDirectory(info.fileName());
                    }
                    else
                    {
                        QDesktopServices::openUrl(
                                    QUrl("file://"+ info.absoluteFilePath()));
                    }
                }
            }
        }
        else if (selectionMode_ == HandEvent::MULTIPLE )
        {
            itemList_[pItem].selected_ = !itemList_[pItem].selected_ ;
        }
    }
    //hand out of grid ====> release everything
    else
    {
        for(int i = 0; i < itemList_.size(); i++)
            itemList_[i].selected_ = false;
    }
}

//Not used
void GlWidget::slotPalmPos(Vector pPos)
{
    palmPos_ = pPos;
}
