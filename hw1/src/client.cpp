#include <cstdlib>

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

using namespace yarp::os;
using namespace yarp::dev;


class ClientMod : public yarp::os::RFModule
{
private:

    // define one output port to comunicate to the server and
    // one input port to receive the trigger to start moving the
    // arm
    // FILL IN THE CODE
    BufferedPort<Bottle>   triggerReceivePort;
    BufferedPort<Bottle>   serverPort;

    double angle, period;
    bool triggered;

public:

    ClientMod(): angle(0.0), period(0.0), triggered(false)
    {}
    /****************************************************/
    bool configPorts()
    {
        // open all ports and check that everything is fine
        // output port: /client/output
        // input port: /client/input

        // FILL IN THE CODE
        if (!triggerReceivePort.open("/client/input"))
        {
            return false;
        }
        if (!serverPort.open("/client/output"))
        {
            return false;
        }
        while (serverPort.getOutputCount() == 0)
        {
            yInfo()<<"Waiting the connection to the server...";
            yarp::os::Time::delay(0.3);
        }


        return true;
    }

    /****************************************************/
    bool configure(ResourceFinder &rf)
    {
        // get "angle" input parameter
        Bottle * b = triggerReceivePort.read();
        angle = b->get(0).asFloat64();;

        // configure the ports
        bool conf = configPorts();
        
        // Check if angle is passed by command line argument
        if (rf.check("angle"))
        {
            angle  = rf.find("angle").asFloat64();
        }

        // Check if period is passed by command line argument
        if (rf.check("period"))
        {
            period = rf.find("period").asFloat64();
        }
        return conf;
    }

    /****************************************************/
    double getPeriod()
    {
        return period;
    }

    /****************************************************/
    bool close()
    {
        // close ports
        // FILL IN THE CODE
        triggerReceivePort.close();
        serverPort.close();
        return true;
    }

    /****************************************************/
    bool interrupt()
    {
        // interrupt ports
        // FILL IN THE CODE
        triggerReceivePort.interrupt();
        serverPort.interrupt();
        return true;
    }

    /****************************************************/
    bool updateModule()
    {
        Bottle* signal = nullptr;
        if (!triggered)
        {
            // read from the input port the signal from the
            // trigger for starting to send data to the server
            // FILL IN CODE
            signal = triggerReceivePort.read();
            triggered = true;
        }
        // once triggered prepare the bottle containing the
        // angle and send it to the server through the
        // output port
        // FILL IN CODE
        
        signal->addFloat64(angle);
        signal->addFloat64(period);
        serverPort.write(signal);
        
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

    ClientMod mod;
    return mod.runModule(rf);
}
