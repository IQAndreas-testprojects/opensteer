// ----------------------------------------------------------------------------
//
//
// OpenSteer -- Steering Behaviors for Autonomous Characters
//
// Copyright (c) 2002-2003, Sony Computer Entertainment America
// Original author: Craig Reynolds <craig_reynolds@playstation.sony.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//
// ----------------------------------------------------------------------------
//
//
// SteerTest
//
// This class encapsulates the state of the SteerTest application and the
// services it provides to its plug-ins.  It is never instantiated, all of
// its members are static (belong to the class as a whole.)
//
// 11-14-02 cwr: created 
//
//
// ----------------------------------------------------------------------------


#include <algorithm>
#include <strstream>
#include "OpenSteer/SteerTest.h"


// ----------------------------------------------------------------------------
// keeps track of both "real time" and "simulation time"


Clock SteerTest::clock;


// ----------------------------------------------------------------------------
// camera automatically tracks selected vehicle


Camera SteerTest::camera;


// ----------------------------------------------------------------------------
// currently selected plug-in (user can choose or cycle through them)


PlugIn* SteerTest::selectedPlugIn = NULL;


// ----------------------------------------------------------------------------
// currently selected vehicle.  Generally the one the camera follows and
// for which additional information may be displayed.  Clicking the mouse
// near a vehicle causes it to become the Selected Vehicle.


AbstractVehicle* SteerTest::selectedVehicle = NULL;


// ----------------------------------------------------------------------------
// phase: identifies current phase of the per-frame update cycle


int SteerTest::phase = SteerTest::overheadPhase;


// ----------------------------------------------------------------------------
// graphical annotation: master on/off switch


bool SteerTest::enableAnnotation = true;


// ----------------------------------------------------------------------------
// XXX apparently MS VC6 cannot handle initialized static const members,
// XXX so they have to be initialized not-inline.


const int SteerTest::overheadPhase = 0;
const int SteerTest::updatePhase = 1;
const int SteerTest::drawPhase = 2;


// ----------------------------------------------------------------------------
// initialize SteerTest application


void printPlugIn (PlugIn& pi) {cout << " " << pi << std::endl;} // XXX


void SteerTest::initialize (void)
{
    // select the default PlugIn
    selectDefaultPlugIn ();

    {
        // XXX this block is for debugging purposes,
        // XXX should it be replaced with something permanent?

        cout << std::endl << "Known plugins:" << std::endl;   // xxx?
        PlugIn::applyToAll (printPlugIn);                     // xxx?
        cout << std::endl;                                    // xxx?

        // identify default PlugIn
        if (!selectedPlugIn) errorExit ("no default PlugIn");
        cout << std::endl << "Default plugin:" << std::endl;  // xxx?
        cout << " " << *selectedPlugIn << std::endl;          // xxx?
        cout << std::endl;                                    // xxx?
    }

    // initialize the default PlugIn
    openSelectedPlugIn ();
}


// ----------------------------------------------------------------------------
// main update function: step simulation forward and redraw scene


void SteerTest::updateSimulationAndRedraw (void)
{
    // update global simulation clock
    clock.update ();

    //  start the phase timer (XXX to accurately measure "overhead" time this
    //  should be in displayFunc, or somehow account for time outside this
    //  routine)
    initPhaseTimers ();

    // run selected PlugIn (with simulation's current time and step size)
    updateSelectedPlugIn (clock.totalSimulationTime,
                          clock.elapsedSimulationTime);

    // redraw selected PlugIn (based on real time)
    redrawSelectedPlugIn (clock.totalRealTime, clock.elapsedRealTime);
}


// ----------------------------------------------------------------------------
// exit SteerTest with a given text message or error code


void SteerTest::errorExit (const char* message)
{
    printMessage (message);
    exit (-1);
}


void SteerTest::exit (int exitCode)
{
    ::exit (exitCode);
}


// ----------------------------------------------------------------------------
// select the default PlugIn


void SteerTest::selectDefaultPlugIn (void)
{
    PlugIn::sortBySelectionOrder ();
    selectedPlugIn = PlugIn::findDefault ();
}


// ----------------------------------------------------------------------------
// select the "next" plug-in, cycling through "plug-in selection order"


void SteerTest::selectNextPlugIn (void)
{
    closeSelectedPlugIn ();
    selectedPlugIn = selectedPlugIn->next ();
    openSelectedPlugIn ();
}


// ----------------------------------------------------------------------------
// handle function keys an a per-plug-in basis


void SteerTest::functionKeyForPlugIn (int keyNumber)
{
    selectedPlugIn->handleFunctionKeys (keyNumber);
}


// ----------------------------------------------------------------------------
// return name of currently selected plug-in


const char* SteerTest::nameOfSelectedPlugIn (void)
{
    return (selectedPlugIn ? selectedPlugIn->name() : "no PlugIn");
}


// ----------------------------------------------------------------------------
// open the currently selected plug-in


void SteerTest::openSelectedPlugIn (void)
{
    camera.reset ();
    selectedVehicle = NULL;
    selectedPlugIn->open ();
    if (selectedVehicle == NULL)
    {
        std::ostrstream message;
        message << "SteerTest::selectedVehicle was NULL after calling ";
        message << nameOfSelectedPlugIn ();
        message << "'s open method (in SteerTest::openSelectedPlugIn)";
        printWarning (message.str());
    }
}


// ----------------------------------------------------------------------------
// do a simulation update for the currently selected plug-in


void SteerTest::updateSelectedPlugIn (const float currentTime,
                                      const float elapsedTime)
{
    // switch to Update phase
    pushPhase (updatePhase);

    // service queued reset request, if any
    doDelayedResetPlugInXXX ();

    // invoke selected PlugIn's Update method
    selectedPlugIn->update (currentTime, elapsedTime);

    // return to previous phase
    popPhase ();
}


// ----------------------------------------------------------------------------
// redraw graphics for the currently selected plug-in


void SteerTest::redrawSelectedPlugIn (const float currentTime,
                                      const float elapsedTime)
{
    // switch to Draw phase
    pushPhase (drawPhase);

    // invoke selected PlugIn's Draw method
    selectedPlugIn->redraw (currentTime, elapsedTime);

    // draw any annotation queued up during selected PlugIn's Update method
    drawAllDeferredLines ();
    drawAllDeferredCirclesOrDisks ();

    // return to previous phase
    popPhase ();
}


// ----------------------------------------------------------------------------
// close the currently selected plug-in


void SteerTest::closeSelectedPlugIn (void)
{
    selectedPlugIn->close ();
    selectedVehicle = NULL;
}


// ----------------------------------------------------------------------------
// reset the currently selected plug-in


void SteerTest::resetSelectedPlugIn (void)
{
    selectedPlugIn->reset ();
}


// ----------------------------------------------------------------------------
// XXX this is used by CaptureTheFlag
// XXX it was moved here from main.cpp on 12-4-02
// XXX I'm not sure if this is a useful feature or a bogus hack
// XXX needs to be reconsidered.


bool gDelayedResetPlugInXXX = false;


void SteerTest::queueDelayedResetPlugInXXX (void)
{
    gDelayedResetPlugInXXX = true;
}


void SteerTest::doDelayedResetPlugInXXX (void)
{
    if (gDelayedResetPlugInXXX)
    {
        resetSelectedPlugIn ();
        gDelayedResetPlugInXXX = false;
    }
}


// ----------------------------------------------------------------------------
// return a group (an STL vector of AbstractVehicle pointers) of all
// vehicles(/agents/characters) defined by the currently selected PlugIn


const AVGroup& SteerTest::allVehiclesOfSelectedPlugIn (void)
{
    return selectedPlugIn->allVehicles ();
}


// ----------------------------------------------------------------------------
// select the "next" vehicle: the one listed after the currently selected one
// in allVehiclesOfSelectedPlugIn


void SteerTest::selectNextVehicle (void)
{
    if (selectedVehicle != NULL)
    {
        // get a container of all vehicles
        const AVGroup& all = allVehiclesOfSelectedPlugIn ();
        const AVIterator first = all.begin();
        const AVIterator last = all.end();

        // find selected vehicle in container
        const AVIterator s = std::find (first, last, selectedVehicle);

        // normally select the next vehicle in container
        selectedVehicle = *(s+1);

        // if we are at the end of the container, select the first vehicle
        if (s == last-1) selectedVehicle = *first;

        // if the search failed, use NULL
        if (s == last) selectedVehicle = NULL;
    }
}


// ----------------------------------------------------------------------------
// select vehicle nearest the given screen position (e.g.: of the mouse)


void SteerTest::selectVehicleNearestScreenPosition (int x, int y)
{
    selectedVehicle = findVehicleNearestScreenPosition (x, y);
}


// ----------------------------------------------------------------------------
// Find the AbstractVehicle whose screen position is nearest the current the
// mouse position.  Returns NULL if mouse is outside this window or if
// there are no AbstractVehicle.


AbstractVehicle* SteerTest::vehicleNearestToMouse (void)
{
    return (mouseInWindow ? 
            findVehicleNearestScreenPosition (mouseX, mouseY) :
            NULL);
}


// ----------------------------------------------------------------------------
// Find the AbstractVehicle whose screen position is nearest the given window
// coordinates, typically the mouse position.  Returns NULL if there are no
// AbstractVehicles.
//
// This works by constructing a line in 3d space between the camera location
// and the "mouse point".  Then it measures the distance from that line to the
// centers of each AbstractVehicle.  It returns the AbstractVehicle whose
// distance is smallest.
//
// xxx Issues: Should the distanceFromLine test happen in "perspective space"
// xxx or in "screen space"?  Also: I think this would be happy to select a
// xxx vehicle BEHIND the camera location.


AbstractVehicle* SteerTest::findVehicleNearestScreenPosition (int x, int y)
{
    // find the direction from the camera position to the given pixel
    const Vec3 direction = directionFromCameraToScreenPosition (x, y);

    // iterate over all vehicles to find the one whose center is nearest the
    // "eye-mouse" selection line
    float minDistance = FLT_MAX;       // smallest distance found so far
    AbstractVehicle* nearest = NULL;   // vehicle whose distance is smallest
    const AVGroup& vehicles = allVehiclesOfSelectedPlugIn();
    for (AVIterator i = vehicles.begin(); i != vehicles.end(); i++)
    {
        // distance from this vehicle's center to the selection line:
        const float d = distanceFromLine ((**i).position(),
                                          camera.position(),
                                          direction);

        // if this vehicle-to-line distance is the smallest so far,
        // store it and this vehicle in the selection registers.
        if (d < minDistance)
        {
            minDistance = d;
            nearest = *i;
        }
    }

    return nearest;
}


// ----------------------------------------------------------------------------
// for storing most recent mouse state


int SteerTest::mouseX = 0;
int SteerTest::mouseY = 0;
bool SteerTest::mouseInWindow = false;


// ----------------------------------------------------------------------------
// set a certain initial camera state used by several plug-ins


void SteerTest::init3dCamera (AbstractVehicle& selected)
{
    position3dCamera (selected);

    camera.fixedDistDistance = cameraTargetDistance;
    camera.fixedDistVOffset = camera2dElevation;

    camera.mode = Camera::cmFixedDistanceOffset;
}


void SteerTest::init2dCamera (AbstractVehicle& selected)
{
    position2dCamera (selected);

    camera.fixedDistDistance = cameraTargetDistance;
    camera.fixedDistVOffset = camera2dElevation;

    camera.mode = Camera::cmFixedDistanceOffset;
}


void SteerTest::position3dCamera (AbstractVehicle& selected)
{
    selectedVehicle = &selected;

    const Vec3 behind = selected.forward() * -cameraTargetDistance;
    camera.setPosition (selected.position() + behind);
    camera.target = selected.position();
}


void SteerTest::position2dCamera (AbstractVehicle& selected)
{
    // position the camera as if in 3d:
    position3dCamera (selected);

    // then adjust for 3d:
    Vec3 position3d = camera.position();
    position3d.y += camera2dElevation;
    camera.setPosition (position3d);
}


// ----------------------------------------------------------------------------
// camera updating utility used by several plug-ins


void SteerTest::updateCamera (const float currentTime,
                              const float elapsedTime,
                              const AbstractVehicle& selected)
{
    camera.vehicleToTrack = &selected;
    camera.update (currentTime, elapsedTime, clock.paused);
}


// ----------------------------------------------------------------------------
// some camera-related default constants


const float SteerTest::camera2dElevation = 8;
const float SteerTest::cameraTargetDistance = 13;
const Vec3 SteerTest::cameraTargetOffset (0,
                                          SteerTest::camera2dElevation,
                                          0);


// ----------------------------------------------------------------------------
// ground plane grid-drawing utility used by several plug-ins


void SteerTest::gridUtility (const Vec3& gridTarget)
{
    // round off target to the nearest multiple of 2 (because the
    // checkboard grid with a pitch of 1 tiles with a period of 2)
    // then lower the grid a bit to put it under 2d annotation lines
    const Vec3 gridCenter ((roundf (gridTarget.x * 0.5) * 2),
                           (roundf (gridTarget.y * 0.5) * 2) - .05f,
                           (roundf (gridTarget.z * 0.5) * 2));

    // colors for checkboard
    const Vec3 gray1 = grayColor (0.27f);
    const Vec3 gray2 = grayColor (0.30f);

    // draw 50x50 checkerboard grid with 50 squares along each side
    drawXZCheckerboardGrid (50, 50, gridCenter, gray1, gray2);

    // alternate style
    // drawXZLineGrid (50, 50, gridCenter, black);
}


// ----------------------------------------------------------------------------
// draws a gray disk on the XZ plane under a given vehicle


void SteerTest::highlightVehicleUtility (const AbstractVehicle& vehicle)
{
    if (&vehicle != NULL)
        drawXZDisk (vehicle.radius(), vehicle.position(), gGray60, 20);
}


// ----------------------------------------------------------------------------
// draws a gray circle on the XZ plane under a given vehicle


void SteerTest::circleHighlightVehicleUtility (const AbstractVehicle& vehicle)
{
    if (&vehicle != NULL)
        drawXZCircle (vehicle.radius() * 1.1, vehicle.position(), gGray60, 20);
}


// ----------------------------------------------------------------------------
// draw a box around a vehicle aligned with its local space
// xxx not used as of 11-20-02


void SteerTest::drawBoxHighlightOnVehicle (const AbstractVehicle& v,
                                           const Vec3 color)
{
    if (&v)
    {
        const float diameter = v.radius() * 2;
        const Vec3 size (diameter, diameter, diameter);
        drawBoxOutline (v, size, color);
    }
}


// ----------------------------------------------------------------------------
// draws a colored circle (perpendicular to view axis) around the center
// of a given vehicle.  The circle's radius is the vehicle's radius times
// radiusMultiplier.


void SteerTest::drawCircleHighlightOnVehicle (const AbstractVehicle& v,
                                              const float radiusMultiplier,
                                              const Vec3 color)
{
    if (&v)
    {
        const Vec3& cPosition = camera.position();
        draw3dCircle  (v.radius() * radiusMultiplier,  // adjusted radius
                       v.position(),                   // center
                       v.position() - cPosition,       // view axis
                       color,                          // drawing color
                       20);                            // circle segments
    }
}


// ----------------------------------------------------------------------------


void SteerTest::printMessage (const char* message)
{
    cout << "SteerTest: " <<  message << std::endl << flush;
}


void SteerTest::printWarning (const char* message)
{
    cout << "SteerTest: Warning: " <<  message << std::endl << flush;
}


// ------------------------------------------------------------------------
// print list of known commands
//
// XXX this list should be assembeled automatically,
// XXX perhaps from a list of "command" objects created at initialization


void SteerTest::keyboardMiniHelp (void)
{
    printMessage ("");
    printMessage ("defined single key commands:");
    printMessage ("  r      restart current PlugIn.");
    printMessage ("  s      select next vehicle.");
    printMessage ("  c      select next camera mode.");
    printMessage ("  Tab    select next PlugIn.");
    printMessage ("  Space  toggle between Run and Pause.");
    printMessage ("  ->     step forward one frame.");
    printMessage ("  Esc    exit.");
    printMessage ("");

    // allow PlugIn to print mini help for the function keys it handles
    selectedPlugIn->printMiniHelpForFunctionKeys ();
}


// ----------------------------------------------------------------------------
// manage SteerTest phase transitions (xxx and maintain phase timers)


int SteerTest::phaseStackIndex = 0;
const int SteerTest::phaseStackSize = 5;
int SteerTest::phaseStack [SteerTest::phaseStackSize];


void SteerTest::pushPhase (const int newPhase)
{
    // update timer for current (old) phase: add in time since last switch
    updatePhaseTimers ();

    // save old phase
    phaseStack[phaseStackIndex++] = phase;

    // set new phase
    phase = newPhase;

    // check for stack overflow
    if (phaseStackIndex >= phaseStackSize) errorExit ("phaseStack overflow");
}


void SteerTest::popPhase (void)
{
    // update timer for current (old) phase: add in time since last switch
    updatePhaseTimers ();

    // restore old phase
    phase = phaseStack[--phaseStackIndex];
}


// ----------------------------------------------------------------------------


float SteerTest::phaseTimerBase = 0;
float SteerTest::phaseTimers [drawPhase+1];


void SteerTest::initPhaseTimers (void)
{
    phaseTimers[drawPhase] = 0;
    phaseTimers[updatePhase] = 0;
    phaseTimers[overheadPhase] = 0;
    phaseTimerBase = clock.totalRealTime;
}


void SteerTest::updatePhaseTimers (void)
{
    const float currentRealTime = clock.realTimeSinceFirstClockUpdate();
    phaseTimers[phase] += currentRealTime - phaseTimerBase;
    phaseTimerBase = currentRealTime;
}


// ----------------------------------------------------------------------------