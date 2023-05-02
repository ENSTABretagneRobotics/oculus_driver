/******************************************************************************
 * oculus_driver driver library for Blueprint Subsea Oculus sonar.
 * Copyright (C) 2020 ENSTA-Bretagne
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *****************************************************************************/

#ifndef _DEF_OCULUS_DRIVER_UTILS_H_
#define _DEF_OCULUS_DRIVER_UTILS_H_

#include <iostream>

#include <boost/asio.hpp>

#include <oculus_driver/Oculus.h>
#include <oculus_driver/print_utils.h>

namespace oculus {

template <typename EndPointT>
inline EndPointT remote_from_status(const OculusStatusMsg& status)
{
    // going through string conversion allows to not care about
    // endianess. (fix this)
    return EndPointT(boost::asio::ip::address_v4::from_string(
        ip_to_string(status.ipAddr)), 52100);
}

inline bool header_valid(const OculusMessageHeader& header)
{
    return header.oculusId == 0x4f53;
}

inline bool is_ping_message(const OculusMessageHeader& header)
{
    return header_valid(header)
        && header.msgId == messageSimplePingResult;
}

inline OculusSimpleFireMessage default_ping_config()
{
    OculusSimpleFireMessage msg;
    std::memset(&msg, 0, sizeof(msg));

    msg.head.oculusId    = OCULUS_CHECK_ID;
    msg.head.msgId       = messageSimpleFire;
    msg.head.srcDeviceId = 0;
    msg.head.dstDeviceId = 0;
    msg.head.payloadSize = sizeof(OculusSimpleFireMessage) - sizeof(OculusMessageHeader);

    msg.masterMode      = 2;
    msg.networkSpeed    = 0xff;
    msg.gammaCorrection = 127;
    msg.pingRate        = pingRateNormal;
    //msg.pingRate        = pingRateHigh;
    msg.range           = 2.54;
    msg.gainPercent     = 50;
    //msg.flags           = 0x19;
    //msg.flags           = 0x09; // seems to be no difference with 0x19
    //msg.flags           = 0x11; // Ping result not implemented and not in SDK (investigate ?)

    msg.flags  = 0x1;  // always in meters
    //msg.flags |= 0x2;  // 16bits data
    msg.flags |= 0x4;  // Sends gain for each range in returned message
    msg.flags |= 0x8;  // simple ping 
    //msg.flags |= 0x40; // 512 beams ???! (found in sdk)

    msg.speedOfSound    = 0.0;
    msg.salinity        = 0.0;
    
    return msg;
}


inline bool check_config_feedback(const OculusSimpleFireMessage& requested,
                                  const OculusSimpleFireMessage& feedback)
{
    // returns true if feedback coherent with requested config.
    if(requested.pingRate == pingRateStandby) {
        // If in standby, expecting a dummy message
        if(feedback.head.msgId == messageDummy)
            return true;
    }
    else {
        // If got a simple ping result, checking relevant parameters
        if(feedback.head.msgId == messageSimplePingResult
           && requested.masterMode       == feedback.masterMode
           // feedback is broken on pingRate field
           //&& requested.pingRate         == feedback.pingRate 
           && requested.gammaCorrection  == feedback.gammaCorrection
           && requested.flags            == feedback.flags
           && requested.range            == feedback.range
           && std::abs(requested.gainPercent  - feedback.gainPercent)  < 1.0e-1)
        {
            //return true; // bypassing checks on sound speed
            // changing soundspeed is very slow (up to 6 seconds, maybe more)

            // For now simple ping is ok. Checking sound speed / salinity
            // parameters If speed of sound is 0.0, the sonar is using salinity
            // to calculate speed of sound.
            if(requested.speedOfSound != 0.0) {
                if(std::abs(requested.speedOfSound - feedback.speedOfSound) < 1.0e-1)
                    return true;
            }
            else {
                if(std::abs(requested.salinity - feedback.salinity) < 1.0e-1)
                    return true;
            }
        }
    }
    return false;
}

inline bool config_changed(const OculusSimpleFireMessage& previous,
                           const OculusSimpleFireMessage& next)
{

    if(previous.masterMode      !=  next.masterMode)      return true;
    if(previous.pingRate        !=  next.pingRate)        return true;
    if(previous.networkSpeed    !=  next.networkSpeed)    return true;
    if(previous.gammaCorrection !=  next.gammaCorrection) return true;
    if(previous.flags           !=  next.flags)           return true;

    if(abs(previous.range        - next.range)        > 0.001) return true;
    if(abs(previous.gainPercent  - next.gainPercent)  > 0.1)   return true;
    if(abs(previous.speedOfSound - next.speedOfSound) > 0.1)   return true;
    if(abs(previous.salinity     - next.salinity)     > 0.1)   return true;

    return false;
}

} //namespace oculus

#endif //_DEF_OCULUS_DRIVER_UTILS_H_
