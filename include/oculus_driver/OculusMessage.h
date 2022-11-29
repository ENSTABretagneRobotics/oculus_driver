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

    // Only the SonarClient and FileReader classes are able to modify this
    // type. This is to ensure consistency between the header_ and data_
    // fields.
    friend class SonarClient;
    friend class FileReader;

    using Ptr        = std::shared_ptr<Message>;
    using ConstPtr   = std::shared_ptr<const Message>;

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

    public:// have to leave constructors public for pybind11

    Message() { std::memset(&header_, 0, sizeof(header_)); }
    Message(const Message& other) :
        timestamp_(other.timestamp_),
        header_(other.header_),
        data_(other.data_)
    {}

    public:

    static Ptr Create()                      { return Ptr(new Message());       }
    static Ptr Create(const Message&  other) { return Ptr(new Message(other));  }
    static Ptr Create(const ConstPtr& other) { return Ptr(new Message(*other)); }

    Ptr copy() const { return Create(*this); }
    
    const OculusMessageHeader&  header()    const { return header_;    }
    const std::vector<uint8_t>& data()      const { return data_;      }
    const TimePoint&            timestamp() const { return timestamp_; }
    
    // getters below here are merely helpers to read the data.
    uint16_t message_id()      const { return header_.msgId;       }
    uint16_t message_version() const { return header_.msgVersion;  }
    uint32_t payload_size()    const { return header_.payloadSize; }
    
    bool is_ping_message() const { return this->message_id() == messageSimplePingResult; }
};

class PingMessage
{
    public:

    using Ptr      = std::shared_ptr<PingMessage>;
    using ConstPtr = std::shared_ptr<const PingMessage>;

    using TimeSource = Message::TimeSource;
    using TimePoint  = Message::TimePoint;

    protected:

    Message::ConstPtr msg_;

    public:// have to leave constructors public for pybind11

    PingMessage(const Message::ConstPtr& msg) : msg_(msg) {}
    
    public:

    const OculusMessageHeader&  header()    const { return msg_->header();    }
    const std::vector<uint8_t>& data()      const { return msg_->data();      }
    const TimePoint&            timestamp() const { return msg_->timestamp(); }

    virtual Ptr copy() const = 0;
    
    virtual uint16_t       range_count()   const = 0;
    virtual uint16_t       bearing_count() const = 0;
    virtual const int16_t* bearing_data()  const = 0;
    virtual const uint8_t* ping_data()     const = 0;
    
    virtual bool    has_gains()   const = 0;
    virtual uint8_t master_mode() const = 0;
    virtual uint8_t sample_size() const = 0;
};

class PingMessage1 : public PingMessage
{
    public:

    using Ptr      = std::shared_ptr<PingMessage1>;
    using ConstPtr = std::shared_ptr<const PingMessage1>;

    //protected:
    public:// have to leave constructors public for pybind11

    PingMessage1(const Message::ConstPtr& msg) :
        PingMessage(msg)
    {
        if(!is_ping_v1(*msg)) {
            throw std::runtime_error(
                "Tried to instanciate a PingMessage1 with the wrong message type.");
        }
    }

    public:

    static bool is_ping_v1(const Message& msg) {
        return msg.message_id() == messageSimplePingResult && msg.message_version() != 2;
    }

    static Ptr Create(const Message::ConstPtr& msg) {
        if(!msg || !is_ping_v1(*msg))
            return nullptr;
        return Ptr(new PingMessage1(msg));
    }

    //virtual Ptr copy() const { return Create(this->msg_->copy()); } // not compiling
    virtual PingMessage::Ptr copy() const { return Create(this->msg_->copy()); }

    const OculusSimplePingResult& metadata() const {
        return *reinterpret_cast<const OculusSimplePingResult*>(this->msg_->data().data());
    }

    virtual uint16_t       range_count()   const { return this->metadata().nRanges; }
    virtual uint16_t       bearing_count() const { return this->metadata().nBeams;  }
    virtual const int16_t* bearing_data()  const {
        return (const int16_t*)(this->data().data() + sizeof(this->metadata()));
    }
    virtual const uint8_t* ping_data() const {
        return this->data().data() + this->metadata().imageOffset;
    }

    virtual bool    has_gains()       const { return this->metadata().fireMessage.flags | 0x4; }
    virtual uint8_t master_mode()     const { return this->metadata().fireMessage.masterMode;  }
    virtual uint8_t sample_size()     const {
        switch(this->metadata().dataSize) {
            case dataSize8Bit:  return 1; break;
            case dataSize16Bit: return 2; break;
            case dataSize24Bit: return 3; break;
            case dataSize32Bit: return 4; break;
            default:
                //invalid value in metadata.dataSize. Deducing from message size.
                auto lineStep = this->metadata().imageSize / this->metadata().nRanges;
                if(lineStep*this->metadata().nRanges != this->metadata().imageSize) {
                    return 0;
                }
                if(this->has_gains())
                    lineStep -= 4;
                auto sampleSize = lineStep / this->metadata().nBeams;
                // Checking integrity
                if(sampleSize*this->metadata().nBeams != lineStep) {
                    return 0;
                }
                return sampleSize;
                break;
        }
    }
};

} //namespace oculus

#endif //_DEF_OCULUS_DRIVER_OCULUS_MESSAGE_H_
