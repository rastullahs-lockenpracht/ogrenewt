#include "OgreNewtonFrameListener.h"


//#include <OgreNoMemoryMacros.h>
#include <CEGUI/CEGUI.h>
//#include <OgreMemoryMacros.h>


OgreNewtonFrameListener::OgreNewtonFrameListener(RenderWindow* win, Camera* cam, SceneManager* mgr, OgreNewt::World* W) :
		ExampleFrameListener(win,cam)
{
	m_World = W;
	mSceneMgr = mgr;

	dragging = false;

	// setup 3D line node.
	mDragLineNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	mDragLine = new Ogre::ManualObject( "__DRAG_LINES__" );


    timer = 0.0f;
    OgreNewt::Debugger &debug(m_World->getDebugger());
    debug.init(mgr);
}

OgreNewtonFrameListener::~OgreNewtonFrameListener(void)
{
}

bool OgreNewtonFrameListener::frameStarted(const FrameEvent &evt)
{
	mKeyboard->capture();
	mMouse->capture();

	// ----------------------------------------
	// CAMERA CONTROLS
	// ----------------------------------------
	if ((mKeyboard->isKeyDown(OIS::KC_LSHIFT)) || (mKeyboard->isKeyDown(OIS::KC_RSHIFT)))
	{
		Vector3 trans, strafe, vec;
		Quaternion quat;

		quat = mCamera->getOrientation();

		vec = Vector3(0.0,0.0,-0.5);
		trans = quat * vec;

		vec = Vector3(0.5,0.0,0.0);
		strafe = quat * vec;

		mCamera->pitch( Degree(mMouse->getMouseState().Y.rel * -0.5) );
        mCamera->setFixedYawAxis(true);
		mCamera->yaw( Degree(mMouse->getMouseState().X.rel * -0.5) );

		if (mKeyboard->isKeyDown(OIS::KC_UP))
			mCamera->moveRelative(trans);

		if (mKeyboard->isKeyDown(OIS::KC_DOWN))
			mCamera->moveRelative(trans * -1.0);

		if (mKeyboard->isKeyDown(OIS::KC_LEFT))
			mCamera->moveRelative(strafe * -1.0);

		if (mKeyboard->isKeyDown(OIS::KC_RIGHT))
			mCamera->moveRelative(strafe);
	}


	// ----------------------------------------
	// DRAGGING!
	// ----------------------------------------

	if (!dragging)
	{

		//user pressing the left mouse button?
		if (mMouse->getMouseState().buttonDown(OIS::MB_Left))
		{
			// perform a raycast!
			// start at the camera, and go for 100 units in the Z direction.
			Ogre::Vector3 start, end;

			CEGUI::Point mouse = CEGUI::MouseCursor::getSingleton().getPosition();
			CEGUI::Renderer* rend = CEGUI::System::getSingleton().getRenderer();
	
			Ogre::Real mx,my;
			mx = mouse.d_x / rend->getWidth();
			my = mouse.d_y / rend->getHeight();
			Ogre::Ray camray = mCamera->getCameraToViewportRay(mx,my);

			start = camray.getOrigin();
			end = camray.getPoint( 100.0 );

			OgreNewt::BasicRaycast* ray = new OgreNewt::BasicRaycast( m_World, start, end, true );
			OgreNewt::BasicRaycast::BasicRaycastInfo info = ray->getFirstHit();

			if (info.mBody)
			{
				// a body was found.  first let's find the point we clicked, in local coordinates of the body.
				

				// while dragging, make sure the body can't fall asleep.
				info.mBody->unFreeze();
				//info.mBody->setAutoFreeze(0);

				Ogre::Vector3 bodpos;
				Ogre::Quaternion bodorient;

				info.mBody->getPositionOrientation( bodpos, bodorient );

				// info.mDistance is in the range [0,1].
				Ogre::Vector3 globalpt = camray.getPoint( 100.0 * info.mDistance );
				Ogre::Vector3 localpt = bodorient.Inverse() * (globalpt - bodpos);

				// now we need to save this point to apply the spring force, I'm using the userData of the bodies in this example.
                // (where is it used? probably not needed here...)
#ifndef OGRENEWT_NO_OGRE_ANY
                info.mBody->setUserData( Ogre::Any(this) );
#else
				info.mBody->setUserData( this );
#endif


				// now change the force callback from the standard one to the one that applies the spring (drag) force.
				info.mBody->setCustomForceAndTorqueCallback<OgreNewtonFrameListener>( &OgreNewtonFrameListener::dragCallback, this );

				// save the relevant drag information.
				dragBody = info.mBody;
				dragDist = (100.0 * info.mDistance);
				dragPoint = localpt;

				dragging = true;
			}

			delete ray;

		}

		if (mDragLine)
			remove3DLine();
	}
	else
	{
		// currently dragging!
		if (!mMouse->getMouseState().buttonDown(OIS::MB_Left))
		{
			// no longer holding mouse button, so stop dragging!
			// remove the special callback, and put it back to standard gravity.
			dragBody->setCustomForceAndTorqueCallback<OgreNewtonFrameListener>( &OgreNewtonFrameListener::standardForceCallback, this );
			//dragBody->setAutoFreeze(1);

			dragBody = NULL;
			dragPoint = Ogre::Vector3::ZERO;
			dragDist = 0.0;

			dragging = false;
		}
	}



    OgreNewt::Debugger& debug(m_World->getDebugger());
    if (mKeyboard->isKeyDown(OIS::KC_F3))
    {
        debug.showDebugInformation();
        debug.startRaycastRecording();
        //debug.clearRaycastsRecorded();
    }
    else
    {
        debug.hideDebugInformation();
        debug.clearRaycastsRecorded();
        debug.stopRaycastRecording();
    }



    if (mKeyboard->isKeyDown(OIS::KC_T))
        m_World->setThreadCount( m_World->getThreadCount() % 2 + 1);


	if (mKeyboard->isKeyDown(OIS::KC_ESCAPE))
		return false;

	return true;
}



void OgreNewtonFrameListener::dragCallback( OgreNewt::Body* me, float timestep, int threadindex )
{
	// first find the global point the mouse is at...
	CEGUI::Point mouse = CEGUI::MouseCursor::getSingleton().getPosition();
	CEGUI::Renderer* rend = CEGUI::System::getSingleton().getRenderer();
	
	Ogre::Real mx,my;
	mx = mouse.d_x / rend->getWidth();
	my = mouse.d_y / rend->getHeight();
	Ogre::Ray camray = mCamera->getCameraToViewportRay( mx, my );

	Ogre::Vector3 campt = camray.getPoint( dragDist );

	// now find the global point on the body:
	Ogre::Quaternion bodorient;
	Ogre::Vector3 bodpos;

	me->getPositionOrientation( bodpos, bodorient );

	Ogre::Vector3 bodpt = (bodorient * dragPoint) + bodpos;

	// apply the spring force!
	Ogre::Vector3 inertia;
	Ogre::Real mass;

	me->getMassMatrix( mass, inertia );

	Ogre::Vector3 dragForce = ((campt - bodpt) * mass * 8.0) - me->getVelocity();

	// draw a 3D line between these points for visual effect :)
    /*
	remove3DLine();
	mDragLine->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST );
	mDragLine->position( campt );
	mDragLine->position( bodpt );
    // ---------------------------- I get an error here using multithreading... -------------------------------
	mDragLine->end();
	mDragLineNode->attachObject( mDragLine );
    */


	// Add the force!
	me->addGlobalForce( dragForce, bodpt );

	Ogre::Vector3 gravity = Ogre::Vector3(0,-9.8,0) * mass;
	me->addForce( gravity );

	// also don't forget buoyancy force.
	me->addBouyancyForce<OgreNewtonFrameListener>( 0.7, 0.5, 0.5, Ogre::Vector3(0.0f,-9.8f,0.0f), &OgreNewtonFrameListener::buoyancyCallback, this );

}

void OgreNewtonFrameListener::standardForceCallback( OgreNewt::Body* me, float timestep, int threadindex )
{
	Ogre::Real mass;
	Ogre::Vector3 inertia;

	me->getMassMatrix( mass, inertia );

	Ogre::Vector3 gravity = Ogre::Vector3(0,-9.8,0) * mass;
	me->addForce( gravity );

	// also don't forget buoyancy force.
	// just pass the acceleration due to gravity, not the force (accel * mass)! 
	me->addBouyancyForce<OgreNewtonFrameListener>( 0.7, 0.5, 0.5, Ogre::Vector3(0.0f,-9.8f,0.0f), &OgreNewtonFrameListener::buoyancyCallback, this );
}


//////////////////////////////////////////////////////////////////////////////////
//		BUOYANCY CALLBACK
/////////////////////////////////////////////////////////////////////////////////
bool OgreNewtonFrameListener::buoyancyCallback( int colID, OgreNewt::Body* me, const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Plane& plane )
{
	// here we need to create an Ogre::Plane object representing the surface of the liquid.  in our case, we're 
	// just assuming a completely flat plane of liquid, however you could use this function to retrieve the plane
	// equation for an animated sea, etc.
	plane = Ogre::Plane( Ogre::Vector3(0,1,0), Ogre::Vector3(0,0,0) );

	return true;
}


void OgreNewtonFrameListener::remove3DLine()
{
	mDragLineNode->detachAllObjects();
	mDragLine->clear();
}

