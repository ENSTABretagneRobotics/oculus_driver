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

#ifndef _DEF_OCULUS_DRIVER_OCULUS_MESSAGE_H_
#define _DEF_OCULUS_DRIVER_OCULUS_MESSAGE_H_

#include <vector>
#include <chrono>
#include <cstring>

#include <oculus_driver/Oculus.h>

namespace oculus {

// Forward declaration for friend class declarations
class SonarClient;
class FileReader;

class Message
{
    public:

    // Only the sonar client class is able to modify this type. This is to
    // ensure consistency between the header_ and data_ fields.
    friend class SonarClient;
    friend class FileReader;

    using TimeSource = std::chrono::system_clock;
    using TimePoint  = typename std::invoke_result<decltype(&TimeSource::now)>::type;

    protected:
    
    TimePoint            timestamp_;
    OculusMessageHeader  header_;
    std::vector<uint8_t> data_;

    void update_from_header() {
        timestamp_ = TimeSource::now();
        data_.resize(header_.payloadSize + sizeof(header_));
        *reinterpret_cast<OculusMessageHeader*>(data_.data()) = header_;
    }
    void update_from_data() {
        header_ = *reinterpret_cast<const OculusMessageHeader*>(data_.data());
    }
    uint8_t* payload_handle() { return data_.data() + sizeof(header_); }

    public:

    Message() {
        std::memset(&header_, 0, sizeof(header_));
    }
    
    const OculusMessageHeader&  header()    const { return header_;    }
    const std::vector<uint8_t>& data()      const { return data_;      }
    const TimePoint&            timestamp() const { return timestamp_; }
    
    // getters below here are merely helpers to read the data.
    uint16_t message_id()   const { return header_.msgId;       }
    uint32_t payload_size() const { return header_.payloadSize; }
    
    bool is_ping_message() const { return this->message_id() == messageSimplePingResult; }
};

} //namespace oculus

#endif //_DEF_OCULUS_DRIVER_OCULUS_MESSAGE_H_
