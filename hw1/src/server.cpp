#include <cstdlib>
#include <vector>
#include <yarp/os/Network.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Port.h>
#include <yarp/os/Property.h>
#include <yarp/os/Time.h>
#include <yarp/os/RFModule.h>

#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IControlMode.h>
#include <yarp/dev/IEncoders.h>
#include <yarp/dev/IPositionControl.h>
#include <yarp/dev/IControlLimits.h>

using namespace yarp::os;
using namespace yarp::dev;

class ServerMod : public yarp::os::RFModule
{
private:

    // define one input port to receive the angle from the client
    // FILL IN THE CODE
    Port                            inputPort;

    PolyDriver                       driver;
    IControlMode                    *imod;
    IEncoders                       *ienc;
    IPositionControl                *ipos;
    IControlLimits                  *ilim;

    int                             ax;
    double                          min, max;
    int                             nAxes;

    Bottle bot;

    //Implement the BufferedPort callback, as soon as new data is coming
    void moveArm(Bottle* bot)
    {
        static bool invert = true;
        if (!bot->get(0).isFloat64())
        {
            yError()<<"Expecting a double on read...";
            return;
        }
        double angle = bot->get(0).asFloat64();

        if(ipos)
        {
            if (invert)
                {
                    angle = -1 * angle;
                    invert = false;
                }
                else
                {
                    invert = true;
                }
            if (angle < min || angle > max)
            {
                yError()<<"The angle is out the joint's limits";
                return;
            }

            ipos->positionMove(ax, angle);
        }
    }

public:

    ServerMod() : imod(nullptr), ienc(nullptr), ipos(nullptr), ilim(nullptr),
      min(0.0), max(0.0), nAxes(0), ax(2)
    {}
    /****************************************************/
    bool configPorts()
    {
        // open the input port /server/input
        // FILL IN THE CODE
        if (!inputPort.open("/server/input"))
        {
            return false;
        }
        return true;
    }

    /****************************************************/
    bool configDevice()
    {
        // configure the options for the driver
        Property option;
        option.put("device","remote_controlboard");
        option.put("remote","/icubSim/left_arm");
        option.put("local","/server");

        // open the driver
        if (!driver.open(option))
        {
            yError()<<"Unable to open the device driver";
            return false;
        }

        // open the views
        if (!driver.view(imod) || !driver.view(ienc) || !driver.view(ipos) || !driver.view(ilim))
        {
           yError()<<"Failed to view one of the interfaces";
           return false;
        }

        // tell the device we aim to control
        // in position mode all the joints
        ienc->getAxes(&nAxes);
        std::vector<int> modes(nAxes,VOCAB_CM_POSITION);
        imod->setControlModes(modes.data());

        // get limits of joint 2
        ilim->getLimits(ax, &min, &max);

        // set ref speed for the joint 2 to 40.0 deg/s
        ipos->setRefSpeed(ax, 40.0);

        return true;
    }

    /****************************************************/
    bool configure(ResourceFinder &rf)
    {
        bool config_ok;

        // configure the device for controlling the head
        config_ok = configDevice();

        // configure the ports
        config_ok = configPorts() && config_ok;

        return config_ok;
    }

    /****************************************************/
    double getPeriod()
    {
        return 0.0;
    }

    /****************************************************/
    bool close()
    {
        // close the port and close the PolyDriver
        inputPort.close();
        driver.close();
        return true;
    }

    /****************************************************/
    bool interrupt()
    {

        // interrupt the port
        inputPort.interrupt();
        return true;
    }

    /****************************************************/
    bool updateModule()
    {
        // read from the input port passing "bot"
        inputPort.read(bot);

        if(&bot){
            // try to move the arm
            moveArm(&bot);
        }
        return true;
    }

};

int main(int argc, char *argv[]) {
    Network yarp;
    if (!yarp.checkNetwork())
    {
        yError()<<"YARP doesn't seem to be available";
        return EXIT_FAILURE;
    }

    ResourceFinder rf;
    rf.setDefaultContext("assignment_motor-control");
    rf.configure(argc,argv);

    ServerMod mod;
    return mod.runModule(rf);
}