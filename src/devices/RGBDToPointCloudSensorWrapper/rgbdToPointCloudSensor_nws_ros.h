/*
 * Copyright (C) 2006-2021 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#ifndef YARP_DEV_RGBDTOPOINTCLOUDSENSOR_NWS_ROS_H
#define YARP_DEV_RGBDTOPOINTCLOUDSENSOR_NWS_ROS_H

#include <vector>
#include <iostream>
#include <string>
#include <sstream>

#include <yarp/os/Port.h>
#include <yarp/os/Time.h>
#include <yarp/os/Stamp.h>
#include <yarp/os/Network.h>
#include <yarp/os/Property.h>
#include <yarp/os/PeriodicThread.h>

#include <yarp/sig/Vector.h>

#include <yarp/dev/IWrapper.h>
#include <yarp/dev/IMultipleWrapper.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IRGBDSensor.h>
#include <yarp/dev/FrameGrabberControlImpl.h>

// ROS stuff
#include <yarp/os/Node.h>
#include <yarp/os/Publisher.h>
#include <yarp/os/Subscriber.h>
#include <yarp/rosmsg/TickTime.h>
#include <yarp/rosmsg/sensor_msgs/PointCloud2.h>

#define DEFAULT_THREAD_PERIOD   0.03 // s

namespace RGBDToPointCloudImpl{
    const std::string frameId_param            = "frame_Id";
    const std::string nodeName_param           = "nodeName";
    const std::string pointCloudTopicName_param     = "pointCloudTopicName";
}
/**
 *  @ingroup dev_impl_wrapper
 *
 * \section RgbdToPointCloudSensor_nws_ros_device_parameters Description of input parameters
 * A Network grabber for kinect-like devices.
 * This device will produce one stream of data for the point cloud 
 * derived fron the combination of the data derived from Framegrabber and IDepthSensor interfaces.
 * See they documentation for more details about each interface.
 *
 *   Parameters required by this device are:
 * | Parameter name         | SubParameter            | Type    | Units          | Default Value | Required                        | Description                                                                                         | Notes |
 * |:----------------------:|:-----------------------:|:-------:|:--------------:|:-------------:|:------------------------------: |:---------------------------------------------------------------------------------------------------:|:-----:|
 * | period                 |      -                  | int     |  ms            |   20          |  No                             | refresh period of the broadcasted values in ms                                                      | default 20ms |
 * | subdevice              |      -                  | string  |  -             |   -           |  alternative to 'attach' action | name of the subdevice to use as a data source                                                       | when used, parameters for the subdevice must be provided as well |
 * | forceInfoSync          |      -                  | string  | bool           |   -           |  no                             | set 'true' to force the timestamp on the camera_info message to match the image one                 |  - |
 * | pointCloudTopicName    |      -                  | string  |  -             |   -           |  Yes                            | set the name for ROS point cloud topic                                                              | must start with a leading '/' |
 * | frame_Id               |      -                  | string  |  -             |               |  Yes                            | set the name of the reference frame                                                                 |                               |
 * | nodeName               |      -                  | string  |  -             |   -           |  Yes                            | set the name for ROS node                                                                           | must start with a leading '/' |
 *
 * ROS message type used is sensor_msgs/Image.msg ( http://docs.ros.org/en/api/sensor_msgs/html/msg/PointCloud2.html)
 * Some example of configuration files:
 *
 * Example of configuration file using .ini format.
 *
 * \code{.unparsed}
 * device RgbdToPointCloudSensor_nws_ros
 * subdevice <RGBDsensor>
 * period 30
 * pointCloudTopicName /<robotName>/RGBDToPointCloud
 * frame_Id /<robotName>/<framed_Id>
 * nodeName /<robotName>/RGBDToPointCloudSensorNode
 * \endcode
 */

class RgbdToPointCloudSensor_nws_ros :
        public yarp::dev::DeviceDriver,
        public yarp::dev::IWrapper,
        public yarp::dev::IMultipleWrapper,
        public yarp::os::PeriodicThread
{
private:
    typedef yarp::sig::ImageOf<yarp::sig::PixelFloat> DepthImage;
    typedef yarp::os::Publisher<yarp::rosmsg::sensor_msgs::PointCloud2> PointCloudTopicType;
    typedef yarp::rosmsg::sensor_msgs::PointCloud2 PointCloud2Type;
    typedef unsigned int                                 UInt;

    enum SensorType{COLOR_SENSOR, DEPTH_SENSOR};

    template <class T>
    struct param
    {
        param(T& inVar, std::string inName)
        {
            var          = &inVar;
            parname      = inName;
        }
        T*              var;
        std::string     parname;
    };

    PointCloudTopicType   rosPublisherPort_pointCloud;
    yarp::os::Node*       rosNode;
    std::string           nodeName;
    std::string           pointCloudTopicName;
    std::string           rosFrameId;
    // images from device
    yarp::sig::FlexImage  colorImage;
    DepthImage            depthImage;
    UInt                  nodeSeq;


    // this is the sub device or the real device

    double                         period;
    std::string                    sensorId;
    yarp::dev::IRGBDSensor*        sensor_p;
    yarp::dev::IFrameGrabberControls* fgCtrl;
    yarp::dev::IRGBDSensor::RGBDSensor_status sensorStatus;
    int                            verbose;
    bool                           forceInfoSync;
    bool                           initialize_ROS(yarp::os::Searchable& config);
    bool                           read(yarp::os::ConnectionReader& connection);

    // Open the wrapper only, the attach method needs to be called before using it
    // Typical usage: yarprobotinterface
    bool                           openDeferredAttach(yarp::os::Searchable& prop);

    // If a subdevice parameter is given, the wrapper will open it and attach to immediately.
    // Typical usage: simulator or command line
    bool                           isSubdeviceOwned;
    yarp::dev::PolyDriver*         subDeviceOwned;
    bool                           openAndAttachSubDevice(yarp::os::Searchable& prop);

    // Synch
    yarp::os::Stamp                colorStamp;
    yarp::os::Stamp                depthStamp;   
    yarp::os::Property             m_conf;

    bool writeData();

    static std::string yarp2RosPixelCode(int code);

public:
    RgbdToPointCloudSensor_nws_ros();
    RgbdToPointCloudSensor_nws_ros(const RgbdToPointCloudSensor_nws_ros&) = delete;
    RgbdToPointCloudSensor_nws_ros(RgbdToPointCloudSensor_nws_ros&&) = delete;
    RgbdToPointCloudSensor_nws_ros& operator=(const RgbdToPointCloudSensor_nws_ros&) = delete;
    RgbdToPointCloudSensor_nws_ros& operator=(RgbdToPointCloudSensor_nws_ros&&) = delete;
    ~RgbdToPointCloudSensor_nws_ros() override;

    bool        open(yarp::os::Searchable &params) override;
    bool        fromConfig(yarp::os::Searchable &params);
    bool        close() override;

    void        setId(const std::string &id);
    std::string getId();

    /**
      * Specify which sensor this thread has to read from.
      */
    bool        attachAll(const yarp::dev::PolyDriverList &p) override;
    bool        detachAll() override;

    bool        attach(yarp::dev::PolyDriver *poly) override;
    bool        attach(yarp::dev::IRGBDSensor *s);
    bool        detach() override;

    bool        threadInit() override;
    void        threadRelease() override;
    void        run() override;
};

#endif   // YARP_DEV_RGBDTOPOINTCLOUDSENSOR_NWS_ROS_H
