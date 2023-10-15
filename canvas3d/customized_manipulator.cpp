#include "customized_manipulator.h"

CustomizedManipulator::CustomizedManipulator()
{
    _flags |= UserInteractionFlags::SET_CENTER_ON_WHEEL_FORWARD_MOVEMENT;
    setVerticalAxisFixed(true);
    setAllowThrow(false);
}

CustomizedManipulator::~CustomizedManipulator()
{

}

bool CustomizedManipulator::handleMouseDrag(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
    addMouseEvent(ea);

    if (performMovement())
        us.requestRedraw();

    us.requestContinuousUpdate(false);
    _thrown = false;

    return true;
}

bool CustomizedManipulator::performMovement()
{
    // return if less then two events have been added
    if (_ga_t0.get() == NULL || _ga_t1.get() == NULL)
        return false;

    // get delta time
    double eventTimeDelta = _ga_t0->getTime() - _ga_t1->getTime();
    if (eventTimeDelta < 0.)
    {
        OSG_WARN << "Manipulator warning: eventTimeDelta = " << eventTimeDelta << std::endl;
        eventTimeDelta = 0.;
    }

    // get deltaX and deltaY
    float dx = _ga_t0->getXnormalized() - _ga_t1->getXnormalized();
    float dy = _ga_t0->getYnormalized() - _ga_t1->getYnormalized();

    // return if there is no movement.
    if (dx == 0. && dy == 0.)
        return false;


    // call appropriate methods
    unsigned int buttonMask = _ga_t1->getButtonMask();
    unsigned int modKeyMask = _ga_t1->getModKeyMask();
    if (buttonMask == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
    {
        return performMovementLeftMouseButton(eventTimeDelta, dx, dy);
    }
    else if ((buttonMask == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON) ||
        (buttonMask == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON && modKeyMask & osgGA::GUIEventAdapter::MODKEY_CTRL) ||
        (buttonMask == (osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON | osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)))
    {
        return performMovementMiddleMouseButton(eventTimeDelta, dx, dy);
    }
    else if (buttonMask == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
    {
        return performMovementRightMouseButton(eventTimeDelta, dx, dy);
    }

    return false;
}

bool CustomizedManipulator::performMovementLeftMouseButton(const double eventTimeDelta, const double dx, const double dy)
{
    // rotate camera
    if (getVerticalAxisFixed())
        rotateWithFixedVertical(dx, dy);
    else
        rotateTrackball(_ga_t0->getXnormalized(), _ga_t0->getYnormalized(),
            _ga_t1->getXnormalized(), _ga_t1->getYnormalized(),
            getThrowScale(eventTimeDelta));
    return true;
}

void CustomizedManipulator::rotateTrackball(const float px0, const float py0,
    const float px1, const float py1, const float scale)
{
    osg::Vec3d axis;
    float angle;

    trackball(axis, angle, px0 + (px1 - px0) * scale, py0 + (py1 - py0) * scale, px0, py0);

    osg::Quat new_rotate;
    new_rotate.makeRotate(angle, axis);

    _rotation = _rotation * new_rotate;
}

bool CustomizedManipulator::handleMouseWheel(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
    osgGA::GUIEventAdapter::ScrollingMotion sm = ea.getScrollingMotion();

    // handle centering
    if (_flags & SET_CENTER_ON_WHEEL_FORWARD_MOVEMENT)
    {

        if (((sm == osgGA::GUIEventAdapter::SCROLL_DOWN && _wheelZoomFactor > 0.)) ||
            ((sm == osgGA::GUIEventAdapter::SCROLL_UP && _wheelZoomFactor < 0.)))
        {

            if (getAnimationTime() <= 0.)
            {
                // center by mouse intersection (no animation)
                setCenterByMousePointerIntersection(ea, us);
            }
            else
            {
                // start new animation only if there is no animation in progress
                if (!isAnimating())
                    startAnimationByMousePointerIntersection(ea, us);

            }

        }
    }

    switch (sm)
    {
        // mouse scroll up event
    case osgGA::GUIEventAdapter::SCROLL_UP:
    {
        // perform zoom
        zoomModel(-_wheelZoomFactor, true);
        us.requestRedraw();
        us.requestContinuousUpdate(isAnimating() || _thrown);
        return true;
    }

    // mouse scroll down event
    case osgGA::GUIEventAdapter::SCROLL_DOWN:
    {
        // perform zoom
        zoomModel(_wheelZoomFactor, true);
        us.requestRedraw();
        us.requestContinuousUpdate(isAnimating() || _thrown);
        return true;
    }

    // unhandled mouse scrolling motion
    default:
        return false;
    }
}


FirstPersionManipulator::FirstPersionManipulator(const osg::Vec3& front, const osg::Vec3& up) :
    m_defaultFront(front), m_defaultUp(up), m_distance(1.0)
{
    
}

bool FirstPersionManipulator::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME) {
        handleFrame(ea, aa);
        return false;
    }

    if (ea.getHandled()) {
        return false;
    }

    switch (ea.getEventType())
    {
    case osgGA::GUIEventAdapter::KEYDOWN:
        handleKeyDown(ea, aa);
        break;
    case osgGA::GUIEventAdapter::MOVE:
    case osgGA::GUIEventAdapter::DRAG:
        handleMouseMove(ea, aa);
        break;
    default:
        break;
    }

    return false;
}

void FirstPersionManipulator::setByMatrix(const osg::Matrixd& matrix)
{

}

void FirstPersionManipulator::setByInverseMatrix(const osg::Matrixd& matrix)
{

}

osg::Matrixd FirstPersionManipulator::getMatrix() const
{
    return osg::Matrixd::translate(0.0, 0.0, m_distance) *
        osg::Matrixd::rotate(m_rotation) *
        osg::Matrixd::translate(m_center);
}

osg::Matrixd FirstPersionManipulator::getInverseMatrix() const
{
    return osg::Matrixd::translate(-m_center) *
        osg::Matrixd::rotate(m_rotation.inverse()) *
        osg::Matrixd::translate(0.0, 0.0, -m_distance);
}

void FirstPersionManipulator::handleFrame(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{

}

void FirstPersionManipulator::handleKeyDown(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    switch (ea.getKey())
    {
    case osgGA::GUIEventAdapter::KEY_W: {
        break;
    }
    case osgGA::GUIEventAdapter::KEY_A: {
        break;
    }
    case osgGA::GUIEventAdapter::KEY_D: {
        break;
    }
    case osgGA::GUIEventAdapter::KEY_S: {
        break;
    }
    default:
        break;
    }
}

void FirstPersionManipulator::handleMouseMove(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (!m_lastMouseEvent.valid()) {
        m_lastMouseEvent = &ea;
        return;
    }

    float x0 = m_lastMouseEvent->getX(), y0 = m_lastMouseEvent->getY();
    float x1 = ea.getX(), y1 = ea.getY();

    rotate(x1 - x0, y1 - y0);

    m_lastMouseEvent = &ea;
}

void FirstPersionManipulator::rotate(float dx, float dy)
{
    osg::Vec2 offset;
    osg::Quat offsetRotation = osg::Quat(dx / 100, osg::Z_AXIS);

    m_rotation *= offsetRotation;
}