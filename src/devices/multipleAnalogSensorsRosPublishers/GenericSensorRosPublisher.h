/*
 * Copyright (C) 2006-2021 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */


#ifndef YARP_DEV_GENERICSENSORSROSPUBLISHER_H
#define YARP_DEV_GENERICSENSORSROSPUBLISHER_H

#include <yarp/os/PeriodicThread.h>
#include <yarp/os/Publisher.h>
#include <yarp/os/Node.h>
#include <yarp/dev/DeviceDriver.h>
#include <yarp/dev/IMultipleWrapper.h>
#include <yarp/dev/MultipleAnalogSensorsInterfaces.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogComponent.h>
#include <yarp/os/LogStream.h>


// The log component is defined in each device, with a specialized name
YARP_DECLARE_LOG_COMPONENT(GENERICSENSORROSPUBLISHER)


/**
 * @ingroup dev_impl_wrapper
 *
 * \brief This abstract template needs to be specialized in a ROS Publisher, for a specific ROS mesagge/sensor type.
 *
 * | YARP device name |
 * |:-----------------:|
 * | `GenericSensorRosPublisher` |
 *
 * The parameters accepted by this device are:
 * | Parameter name | SubParameter   | Type    | Units          | Default Value    | Required                    | Description                                                       | Notes |
 * |:--------------:|:--------------:|:-------:|:--------------:|:----------------:|:--------------------------: |:-----------------------------------------------------------------:|:-----:|
 * | topic          |      -         | string  | -              |   -              | Yes                         | The name of the ROS topic opened by this device.                  | MUST start with a '/' character |
 * | node_name      |      -         | string  | -              | $topic + "_node" | No                          | The name of the ROS node opened by this device                    | Autogenerated by default |
 * | period         |      -         | double  | s              |   -              | Yes                         | Refresh period of the broadcasted values in seconds               |  |
 */

template <class ROS_MSG>
class GenericSensorRosPublisher :
        public yarp::os::PeriodicThread,
        public yarp::dev::DeviceDriver,
        public yarp::dev::IMultipleWrapper
{
protected:
    double            m_periodInS{0.01};
    std::string       m_publisherName;
    std::string       m_rosNodeName;
    yarp::os::Node*   m_rosNode;
    yarp::os::Publisher<ROS_MSG> m_publisher;
    yarp::dev::PolyDriver* m_poly;
    size_t                 m_msg_counter;
    double                 m_timestamp;
    std::string            m_framename;
    const size_t           m_sens_index = 0;
    yarp::dev::PolyDriver  m_subdevicedriver;

public:
    GenericSensorRosPublisher();
    virtual ~GenericSensorRosPublisher();

    /* DevideDriver methods */
    bool open(yarp::os::Searchable &params) override;
    bool close() override;

    /* IMultipleWrapper methods */
    bool attachAll(const yarp::dev::PolyDriverList &p) override;
    bool detachAll() override;

    /* PeriodicRateThread methods */
    void threadRelease() override;
    void run() override;

protected:
    virtual bool viewInterfaces() = 0;
};

template <class ROS_MSG>
GenericSensorRosPublisher<ROS_MSG>::GenericSensorRosPublisher() :
    PeriodicThread(0.02)
{
    m_rosNode = nullptr;
    m_poly = nullptr;
    m_msg_counter=0;
    m_timestamp=0;
}

template <class ROS_MSG>
GenericSensorRosPublisher<ROS_MSG>::~GenericSensorRosPublisher() = default;

template <class ROS_MSG>
bool GenericSensorRosPublisher<ROS_MSG>::open(yarp::os::Searchable & config)
{
    if (!config.check("topic")) {
        yCError(GENERICSENSORROSPUBLISHER, "Missing `topic` parameter, exiting.");
        return false;
    }

    if (!config.check("period")) {
        yCError(GENERICSENSORROSPUBLISHER, "Missing `period` parameter, exiting.");
        return false;
    }

    if (config.find("period").isFloat32()==false && config.find("period").isFloat64()==false) {
        yCError(GENERICSENSORROSPUBLISHER, "`period` parameter is present but it is not a float, exiting.");
        return false;
    }

    m_periodInS = config.find("period").asFloat64();

    if (m_periodInS <= 0) {
        yCError(GENERICSENSORROSPUBLISHER, "`period` parameter is present (%f) but it is not a positive value, exiting.", m_periodInS);
        return false;
    }

    std::string name = config.find("topic").asString();

    // TODO(traversaro) Add port name validation when ready,
    // see https://github.com/robotology/yarp/pull/1508

    m_rosNodeName = name+ "_node";
    m_publisherName = name;

    if (config.check("node_name"))
    {
        m_rosNodeName = config.find("node_name").asString();
    }

    if (m_rosNodeName == "")
    {
        yCError(GENERICSENSORROSPUBLISHER) << "Invalid node name: " << m_rosNodeName;
        return false;
    }

    m_rosNode = new yarp::os::Node(m_rosNodeName); // add a ROS node

    if (m_rosNode == nullptr) {
        yCError(GENERICSENSORROSPUBLISHER) << "Opening " << m_rosNodeName << " Node, check your yarp-ROS network configuration\n";
        return false;
    }

    if (!m_publisher.topic(m_publisherName)) {
        yCError(GENERICSENSORROSPUBLISHER) << "Opening " << m_publisherName << " Topic, check your yarp-ROS network configuration\n";
        return false;
    }

    if (config.check("subdevice"))
    {
        yarp::os::Property       p;
        yarp::dev::PolyDriverList driverlist;
        p.fromString(config.toString(), false);
        p.put("device", config.find("subdevice").asString());

        if (!m_subdevicedriver.open(p) || !m_subdevicedriver.isValid())
        {
            yCError(GENERICSENSORROSPUBLISHER) << "Failed to open subdevice.. check params";
            return false;
        }

        driverlist.push(&m_subdevicedriver, "1");
        if (!attachAll(driverlist))
        {
            yCError(GENERICSENSORROSPUBLISHER) << "Failed to open subdevice.. check params";
            return false;
        }
    }

    return true;
}

template <class ROS_MSG>
bool GenericSensorRosPublisher<ROS_MSG>::close()
{
    return this->detachAll();

    m_publisher.close();
    if (m_rosNode)
    {
        delete m_rosNode;
        m_rosNode = nullptr;
    }
}

template <class ROS_MSG>
bool GenericSensorRosPublisher<ROS_MSG>::attachAll(const yarp::dev::PolyDriverList & p)
{
    // Attach the device
    if (p.size() > 1)
    {
        yCError(GENERICSENSORROSPUBLISHER, "This device only supports exposing a "
            "single MultipleAnalogSensors device on YARP ports, but %d devices have been passed in attachAll.",
            p.size());
        yCError(GENERICSENSORROSPUBLISHER, "Please use the multipleanalogsensorsremapper device to combine several device in a new device.");
        detachAll();
        return false;
    }

    if (p.size() == 0)
    {
        yCError(GENERICSENSORROSPUBLISHER, "No device passed to attachAll, please pass a device to expose on YARP ports.");
        return false;
    }

    m_poly = p[0]->poly;

    if (!m_poly)
    {
        yCError(GENERICSENSORROSPUBLISHER, "Null pointer passed to attachAll.");
        return false;
    }

    // View all the interfaces
    bool ok = viewInterfaces();

    // Set rate period
    ok &= this->setPeriod(m_periodInS);
    ok &= this->start();

    return ok;
}

template <class ROS_MSG>
bool GenericSensorRosPublisher<ROS_MSG>::detachAll()
{
    // Stop the thread on detach
    if (this->isRunning()) {
        this->stop();
    }
    return true;
}

template <class ROS_MSG>
void GenericSensorRosPublisher<ROS_MSG>::run()
{
}

template <class ROS_MSG>
void GenericSensorRosPublisher<ROS_MSG>::threadRelease()
{
    return;
}

#endif
