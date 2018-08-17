/*
 * Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "Navigation2DClient.h"
#include <yarp/dev/INavigation2D.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/LockGuard.h>
#include <cmath>

/*! \file Navigation2DClient.cpp */

using namespace yarp::dev;
using namespace yarp::os;
using namespace yarp::sig;


//------------------------------------------------------------------------------------------------------------------------------

bool yarp::dev::Navigation2DClient::open(yarp::os::Searchable &config)
{
    m_local_name.clear();
    m_navigation_server_name.clear();
    m_map_locations_server_name.clear();
    m_localization_server_name.clear();

    m_local_name           = config.find("local").asString().c_str();
    m_navigation_server_name = config.find("navigation_server").asString().c_str();
    m_map_locations_server_name = config.find("map_locations_server").asString().c_str();
    m_localization_server_name = config.find("localization_server").asString().c_str();

    if (m_local_name == "")
    {
        yError("Navigation2DClient::open() error you have to provide a valid 'local' param");
        return false;
    }

    if (m_navigation_server_name == "")
    {
        yError("Navigation2DClient::open() error you have to provide a valid 'navigation_server' param");
        return false;
    }

    if (m_map_locations_server_name == "")
    {
        yError("Navigation2DClient::open() error you have to provide valid 'map_locations_server' param");
        return false;
    }

    if (m_localization_server_name == "")
    {
        yError("Navigation2DClient::open() error you have to provide valid 'localization_server' param");
        return false;
    }

    if (config.check("period"))
    {
        m_period = config.find("period").asInt32();
    }
    else
    {
        m_period = 10;
        yWarning("Navigation2DClient: using default period of %d ms" , m_period);
    }

    std::string
            local_rpc_1,
            local_rpc_2,
            local_rpc_3,
            remote_rpc_1,
            remote_rpc_2,
            remote_rpc_3,
            remote_streaming_name,
            local_streaming_name;

    local_rpc_1           = m_local_name           + "/navigation/rpc";
    local_rpc_2           = m_local_name           + "/locations/rpc";
    local_rpc_3           = m_local_name           + "/localization/rpc";
    remote_rpc_1          = m_navigation_server_name + "/rpc";
    remote_rpc_2          = m_map_locations_server_name + "/rpc";
    remote_rpc_3          = m_localization_server_name + "/rpc";
    remote_streaming_name = m_localization_server_name + "/stream:o";
    local_streaming_name  = m_local_name           + "/stream:i";

    if (!m_rpc_port_navigation_server.open(local_rpc_1.c_str()))
    {
        yError("Navigation2DClient::open() error could not open rpc port %s, check network", local_rpc_1.c_str());
        return false;
    }

    if (!m_rpc_port_map_locations_server.open(local_rpc_2.c_str()))
    {
        yError("Navigation2DClient::open() error could not open rpc port %s, check network", local_rpc_2.c_str());
        return false;
    }

    if (!m_rpc_port_localization_server.open(local_rpc_3.c_str()))
    {
        yError("Navigation2DClient::open() error could not open rpc port %s, check network", local_rpc_3.c_str());
        return false;
    }

    /*
    //currently unused
    bool ok=Network::connect(remote_streaming_name.c_str(), local_streaming_name.c_str(), "tcp");
    if (!ok)
    {
        yError("Navigation2DClient::open() error could not connect to %s", remote_streaming_name.c_str());
        return false;
    }*/

    bool ok = true;

    ok = Network::connect(local_rpc_1.c_str(), remote_rpc_1.c_str());
    if (!ok)
    {
        yError("Navigation2DClient::open() error could not connect to %s", remote_rpc_1.c_str());
        return false;
    }

    ok = Network::connect(local_rpc_2.c_str(), remote_rpc_2.c_str());
    if (!ok)
    {
        yError("Navigation2DClient::open() error could not connect to %s", remote_rpc_2.c_str());
        return false;
    }

    ok = Network::connect(local_rpc_3.c_str(), remote_rpc_3.c_str());
    if (!ok)
    {
        yError("Navigation2DClient::open() error could not connect to %s", remote_rpc_3.c_str());
        return false;
    }

    return true;
}

bool yarp::dev::Navigation2DClient::close()
{
    m_rpc_port_navigation_server.close();
    m_rpc_port_map_locations_server.close();
    m_rpc_port_localization_server.close();
    return true;
}

bool yarp::dev::Navigation2DClient::getNavigationStatus(NavigationStatusEnum& status)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GET_STATUS);
    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::getNavigationStatus() received error from navigation server";
            return false;
        }
        else
        {
            status = (NavigationStatusEnum) resp.get(1).asInt32();
            return true;
        }
    }
    else
    {
        yError() << "Navigation2DClient::getNavigationStatus() error on writing on rpc port";
        return false;
    }
    return true;
}


bool yarp::dev::Navigation2DClient::gotoTargetByAbsoluteLocation(Map2DLocation loc)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GOTOABS);
    b.addString(loc.map_id);
    b.addFloat64(loc.x);
    b.addFloat64(loc.y);
    b.addFloat64(loc.theta);
    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::gotoTargetByAbsoluteLocation() received error from navigation server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::gotoTargetByAbsoluteLocation() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::gotoTargetByLocationName(std::string location_name)
{
    yarp::os::Bottle b_loc;
    yarp::os::Bottle resp_loc;
    yarp::os::Bottle b_nav;
    yarp::os::Bottle resp_nav;

    b_loc.addVocab(VOCAB_INAVIGATION);
    b_loc.addVocab(VOCAB_NAV_GET_LOCATION);
    b_loc.addString(location_name);

    bool ret = true;
    ret =  m_rpc_port_map_locations_server.write(b_loc, resp_loc);
    if (ret)
    {
        if (resp_loc.get(0).asVocab() != VOCAB_OK || resp_loc.size() != 5)
        {
            yError() << "Navigation2DClient::gotoTargetByLocationName() received error from locations server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::gotoTargetByLocationName() error on writing on rpc port";
        return false;
    }

    Map2DLocation loc;
    loc.map_id = resp_loc.get(1).asString();
    loc.x = resp_loc.get(2).asFloat64();
    loc.y = resp_loc.get(3).asFloat64();
    loc.theta = resp_loc.get(4).asFloat64();

    b_nav.addVocab(VOCAB_INAVIGATION);
    b_nav.addVocab(VOCAB_NAV_GOTOABS);
    b_nav.addString(loc.map_id);
    b_nav.addFloat64(loc.x);
    b_nav.addFloat64(loc.y);
    b_nav.addFloat64(loc.theta);

    ret = m_rpc_port_navigation_server.write(b_nav, resp_nav);
    if (ret)
    {
        if (resp_nav.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::gotoTargetByLocationName() received error from navigation server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::gotoTargetByLocationName() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::gotoTargetByRelativeLocation(double x, double y)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GOTOREL);
    b.addFloat64(x);
    b.addFloat64(y);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::gotoTargetByRelativeLocation() received error from navigation server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::gotoTargetByRelativeLocation() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::gotoTargetByRelativeLocation(double x, double y, double theta)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GOTOREL);
    b.addFloat64(x);
    b.addFloat64(y);
    b.addFloat64(theta);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::gotoTargetByRelativeLocation() received error from navigation server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::gotoTargetByRelativeLocation() error on writing on rpc port";
        return false;
    }
    return true;
}

bool  yarp::dev::Navigation2DClient::setInitialPose(Map2DLocation& loc)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_SET_INITIAL_POS);
    b.addString(loc.map_id);
    b.addFloat64(loc.x);
    b.addFloat64(loc.y);
    b.addFloat64(loc.theta);

    bool ret = m_rpc_port_localization_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::setInitialPose() received error from localization server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::setInitialPose() error on writing on rpc port";
        return false;
    }
    return true;
}

bool  yarp::dev::Navigation2DClient::getCurrentPosition(Map2DLocation& loc)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GET_CURRENT_POS);

    bool ret = m_rpc_port_localization_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK || resp.size() != 5)
        {
            yError() << "Navigation2DClient::getCurrentPosition() received error from localization server";
            return false;
        }
        else
        {
            loc.map_id = resp.get(1).asString();
            loc.x = resp.get(2).asFloat64();
            loc.y = resp.get(3).asFloat64();
            loc.theta = resp.get(4).asFloat64();
            return true;
        }
    }
    else
    {
        yError() << "Navigation2DClient::getCurrentPosition() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::suspendNavigation(const double time_s)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_SUSPEND);
    b.addFloat64(time_s);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::suspendNavigation() received error from navigation server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::suspendNavigation() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::getAbsoluteLocationOfCurrentTarget(Map2DLocation &loc)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GET_ABS_TARGET);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK || resp.size() != 5)
        {
            yError() << "Navigation2DClient::getAbsoluteLocationOfCurrentTarget() received error from navigation server";
            return false;
        }
        else
        {
            loc.map_id = resp.get(1).asString();
            loc.x = resp.get(2).asFloat64();
            loc.y = resp.get(3).asFloat64();
            loc.theta = resp.get(4).asFloat64();
            return true;
        }
    }
    else
    {
        yError() << "Navigation2DClient::getAbsoluteLocationOfCurrentTarget() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::getNameOfCurrentTarget(std::string& location_name)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GET_NAME_TARGET);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::getNameOfCurrentTarget() received error from server";
            return false;
        }
        else
        {
            location_name = resp.get(1).asString();
            return true;
        }
    }
    else
    {
        yError() << "Navigation2DClient::getNameOfCurrentTarget() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::getRelativeLocationOfCurrentTarget(double& x, double& y, double& theta)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GET_REL_TARGET);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK || resp.size()!=4)
        {
            yError() << "Navigation2DClient::getRelativeLocationOfCurrentTarget() received error from navigation server";
            return false;
        }
        else
        {
            x = resp.get(1).asFloat64();
            y = resp.get(2).asFloat64();
            theta = resp.get(3).asFloat64();
            return true;
        }
    }
    else
    {
        yError() << "Navigation2DClient::getRelativeLocationOfCurrentTarget() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::storeCurrentPosition(std::string location_name)
{
    yarp::os::Bottle b_nav;
    yarp::os::Bottle resp_nav;
    yarp::os::Bottle b_loc;
    yarp::os::Bottle resp_loc;
    Map2DLocation loc;

    b_nav.addVocab(VOCAB_INAVIGATION);
    b_nav.addVocab(VOCAB_NAV_GET_CURRENT_POS);
    bool ret_nav = m_rpc_port_navigation_server.write(b_nav, resp_nav);
    if (ret_nav)
    {
        if (resp_nav.get(0).asVocab() != VOCAB_OK || resp_nav.size()!=5)
        {
            yError() << "Navigation2DClient::storeCurrentPosition() received error from locations server";
            return false;
        }
        else
        {
            loc.map_id = resp_nav.get(1).asString();
            loc.x = resp_nav.get(2).asFloat64();
            loc.y = resp_nav.get(3).asFloat64();
            loc.theta = resp_nav.get(4).asFloat64();
        }
    }
    else
    {
        yError() << "Navigation2DClient::storeCurrentPosition() error on writing on rpc port";
        return false;
    }

    b_loc.addVocab(VOCAB_INAVIGATION);
    b_loc.addVocab(VOCAB_NAV_STORE_ABS);
    b_loc.addString(location_name);
    b_loc.addString(loc.map_id);
    b_loc.addFloat64(loc.x);
    b_loc.addFloat64(loc.y);
    b_loc.addFloat64(loc.theta);

    bool ret_loc = m_rpc_port_map_locations_server.write(b_loc, resp_loc);
    if (ret_loc)
    {
        if (resp_loc.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::storeCurrentPosition() received error from locations server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::storeCurrentPosition() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::storeLocation(std::string location_name, Map2DLocation loc)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_STORE_ABS);
    b.addString(location_name);
    b.addString(loc.map_id);
    b.addFloat64(loc.x);
    b.addFloat64(loc.y);
    b.addFloat64(loc.theta);

    bool ret = m_rpc_port_map_locations_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::storeLocation() received error from locations server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::storeLocation() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::getLocationsList(std::vector<std::string>& locations)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GET_LOCATION_LIST);

    bool ret = m_rpc_port_map_locations_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::getLocationsList() received error from locations server";
            return false;
        }
        else
        {
            Bottle* list = resp.get(1).asList();
            if (list)
            {
                locations.clear();
                for (size_t i = 0; i < list->size(); i++)
                {
                    locations.push_back(list->get(i).asString());
                }
                return true;
            }
            else
            {
                yError() << "Navigation2DClient::getLocationsList() error while reading from locations server";
                return false;
            }
        }
    }
    else
    {
        yError() << "Navigation2DClient::getLocationsList() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::getLocation(std::string location_name, Map2DLocation& loc)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GET_LOCATION);
    b.addString(location_name);

    bool ret = m_rpc_port_map_locations_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::getLocation() received error from locations server";
            return false;
        }
        else
        {
            loc.map_id = resp.get(1).asString();
            loc.x = resp.get(2).asFloat64();
            loc.y = resp.get(3).asFloat64();
            loc.theta = resp.get(4).asFloat64();
        }
    }
    else
    {
        yError() << "Navigation2DClient::getLocation() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::deleteLocation(std::string location_name)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_DELETE);
    b.addString(location_name);

    bool ret = m_rpc_port_map_locations_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::deleteLocation() received error from locations server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::deleteLocation() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::clearAllLocations()
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_CLEAR);

    bool ret = m_rpc_port_map_locations_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::clearAllLocations() received error from locations server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::clearAllLocations() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::stopNavigation()
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_STOP);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::stopNavigation() received error from navigation server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::stopNavigation() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::resumeNavigation()
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_RESUME);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::resumeNavigation() received error from navigation server";
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::resumeNavigation() error on writing on rpc port";
        return false;
    }
    return true;
}

bool   yarp::dev::Navigation2DClient::getAllNavigationWaypoints(std::vector<yarp::dev::Map2DLocation>& waypoints)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GET_NAVIGATION_WAYPOINTS);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::getNavigationWaypoints() received error from navigation server";
            return false;
        }
        else if (resp.get(1).isList() && resp.get(1).asList()->size()>0)
        {
            waypoints.clear();
            for (size_t i = 0; i < resp.get(1).asList()->size(); i++)
            {
                yarp::dev::Map2DLocation loc;
                loc.map_id = resp.get(1).asString();
                loc.x = resp.get(2).asFloat64();
                loc.y = resp.get(3).asFloat64();
                loc.theta = resp.get(4).asFloat64();
                waypoints.push_back(loc);
            }
            return true;
        }
        else
        {
            //not available
            waypoints.clear();
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::getCurrentPosition() error on writing on rpc port";
        return false;
    }
    return true;
}

bool   yarp::dev::Navigation2DClient::getCurrentNavigationWaypoint(yarp::dev::Map2DLocation& curr_waypoint)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_NAV_GET_CURRENT_WAYPOINT);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::getCurrentNavigationWaypoint() received error from navigation server";
            return false;
        }
        else if (resp.size() != 5)
        {
            curr_waypoint.map_id = resp.get(1).asString();
            curr_waypoint.x      = resp.get(2).asFloat64();
            curr_waypoint.y      = resp.get(3).asFloat64();
            curr_waypoint.theta  = resp.get(4).asFloat64();
            return true;
        }
        else
        {
            //not available
            curr_waypoint.map_id = "invalid";
            curr_waypoint.x = std::nan("");
            curr_waypoint.y = std::nan("");
            curr_waypoint.theta = std::nan("");
            return false;
        }
    }
    else
    {
        yError() << "Navigation2DClient::getCurrentPosition() error on writing on rpc port";
        return false;
    }
    return true;
}

bool yarp::dev::Navigation2DClient::getCurrentNavigationMap(yarp::dev::NavigationMapTypeEnum map_type, yarp::dev::MapGrid2D& map)
{
    yarp::os::Bottle b;
    yarp::os::Bottle resp;

    b.addVocab(VOCAB_INAVIGATION);
    b.addVocab(VOCAB_GET_NAV_MAP);
    b.addVocab(map_type);

    bool ret = m_rpc_port_navigation_server.write(b, resp);
    if (ret)
    {
        if (resp.get(0).asVocab() != VOCAB_OK)
        {
            yError() << "Navigation2DClient::getCurrentNavigationMap() received error from server";
            return false;
        }
        else
        {
            Value& b = resp.get(1);
            if (Property::copyPortable(b, map))
            {
                return true;
            }
            else
            {
                yError() << "Navigation2DClient::getCurrentNavigationMap() failed copyPortable()";
                return false;
            }
        }
    }
    else
    {
        yError() << "Navigation2DClient::getCurrentNavigationMap() error on writing on rpc port";
        return false;
    }
    return true;
}


yarp::dev::DriverCreator *createNavigation2DClient()
{
    return new DriverCreatorOf<Navigation2DClient>
               (
                   "navigation2DClient",
                   "",
                   "navigation2DClient"
               );
}
