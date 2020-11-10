/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */


#ifndef YARP_DEV_MAGFIELDROSPUBLISHER_H
#define YARP_DEV_MAGFIELDROSPUBLISHER_H

#include "GenericSensorRosPublisher.h"
#include <yarp/rosmsg/sensor_msgs/MagneticField.h>

    /**
 * @ingroup dev_impl_wrapper
 *
 * \brief `MagneticFieldRosPublisher`: This wrapper connects to a device and publishes a ROS topic of type sensor_msgs::MagneticField.
 *
 * | YARP device name |
 * |:-----------------:|
 * | `MagneticFieldRosPublisher` |
 *
 * The parameters accepted by this device are:
 * | Parameter name | SubParameter   | Type    | Units          | Default Value    | Required                    | Description                                                       | Notes |
 * |:--------------:|:--------------:|:-------:|:--------------:|:----------------:|:--------------------------: |:-----------------------------------------------------------------:|:-----:|
 * | topic          |      -         | string  | -              |   -              | Yes                         | The name of the ROS topic opened by this device.                  | MUST start with a '/' character |
 * | node_name      |      -         | string  | -              | $topic + "_node" | No                          | The name of the ROS node opened by this device                    | Autogenerated by default |
 * | period         |      -         | double  | s              |   -              | Yes                         | Refresh period of the broadcasted values in seconds               |  |
 */
class MagneticFieldRosPublisher : public GenericSensorRosPublisher< yarp::rosmsg::sensor_msgs::MagneticField>
{
protected:
    // Interface of the wrapped device
    yarp::dev::IThreeAxisMagnetometers* m_iThreeAxisMagnetometers{nullptr};

public:
    using GenericSensorRosPublisher<yarp::rosmsg::sensor_msgs::MagneticField>::GenericSensorRosPublisher;

    using GenericSensorRosPublisher<yarp::rosmsg::sensor_msgs::MagneticField>::open;
    using GenericSensorRosPublisher<yarp::rosmsg::sensor_msgs::MagneticField>::close;

    using GenericSensorRosPublisher<yarp::rosmsg::sensor_msgs::MagneticField>::attachAll;
    using GenericSensorRosPublisher<yarp::rosmsg::sensor_msgs::MagneticField>::detachAll;

    /* PeriodicRateThread methods */
    void run() override;

protected:
    virtual bool viewInterfaces() override;
};

#endif
