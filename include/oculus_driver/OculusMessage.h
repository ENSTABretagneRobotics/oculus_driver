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
#include <oculus_driver/utils.h>

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
    static Ptr Create(unsigned int size, const uint8_t* data, 
                      const TimePoint& stamp = TimePoint())
    {
        OculusMessageHeader header;
        std::memcpy(&header, data, sizeof(OculusMessageHeader));
        if(!header_valid(header) || size != header.payloadSize + sizeof(OculusMessageHeader)) {
            return nullptr;
        }
        auto res = Create();
        res->header_ = header;
        res->timestamp_ = stamp;
        res->data_.resize(size);
        std::memcpy(res->data_.data(), data, size);
        return res;
    }

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

class PingWrapper
{
    public:

    using Ptr      = std::shared_ptr<PingWrapper>;
    using ConstPtr = std::shared_ptr<const PingWrapper>;

    using TimeSource = Message::TimeSource;
    using TimePoint  = Message::TimePoint;

    protected:

    Message::ConstPtr msg_;

    PingWrapper(const Message::ConstPtr& msg) : msg_(msg) {
        if(!msg_) {
            throw std::runtime_error("Trying to make a PingMessage out of empty data.");
        }
        if(!msg_->is_ping_message()) {
            throw std::runtime_error("Trying to make a PingMessage out of non-ping data.");
        }
    }
    
    public:

    Message::ConstPtr           message()   const { return msg_;              }
    const OculusMessageHeader&  header()    const { return msg_->header();    }
    const std::vector<uint8_t>& data()      const { return msg_->data();      }
    const TimePoint&            timestamp() const { return msg_->timestamp(); }

    uint32_t step() const {
        return ((this->has_gains() ? 4 : 0) + this->bearing_count()*this->sample_size());
    }

    virtual Ptr copy() const = 0;

    virtual uint16_t       range_count()   const = 0;
    virtual uint16_t       bearing_count() const = 0;
    virtual const int16_t* bearing_data()  const = 0;
    virtual const uint8_t* ping_data()     const = 0;
    virtual uint32_t       ping_data_size()     const = 0;
    
    virtual bool    has_gains()   const = 0;
    virtual uint8_t master_mode() const = 0;
    virtual uint8_t sample_size() const = 0;
    

    virtual uint32_t ping_index()          const = 0;
    virtual uint32_t ping_firing_date()    const = 0;
    virtual double   range()               const = 0;
    virtual double   gain_percent()        const = 0;
    virtual double   frequency()           const = 0;
    virtual double   speed_of_sound_used() const = 0;
    virtual double   range_resolution()    const = 0;
    virtual double   temperature()         const = 0;
    virtual double   pressure()            const = 0;
};

class PingWrapper1 : public PingWrapper
{
    public:

    using Ptr      = std::shared_ptr<PingWrapper1>;
    using ConstPtr = std::shared_ptr<const PingWrapper1>;

    protected:

    PingWrapper1(const Message::ConstPtr& msg) :
        PingWrapper(msg)
    {
        if(msg->message_version() == 2) {
            throw std::runtime_error(
                "Tried to instanciate a PingWrapper1 with data from a PingWrapper2");
        }
    }

    public:

    static Ptr Create(const Message::ConstPtr& msg) { return Ptr(new PingWrapper1(msg)); }

    //virtual Ptr copy() const { return Create(this->msg_->copy()); } // not compiling
    virtual PingWrapper::Ptr copy() const { return Create(this->msg_->copy()); }

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
    virtual uint32_t ping_data_size() const { return this->metadata().imageSize; }

    virtual bool    has_gains()       const { return this->metadata().fireMessage.flags & 0x4; }
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

    virtual uint32_t ping_index()          const { return this->metadata().pingId;                  }
    virtual uint32_t ping_firing_date()    const { return this->metadata().pingStartTime;           }
    virtual double   range()               const { return this->metadata().fireMessage.range;       }
    virtual double   gain_percent()        const { return this->metadata().fireMessage.gainPercent; } 
    virtual double   frequency()           const { return this->metadata().frequency;               }
    virtual double   speed_of_sound_used() const { return this->metadata().speeedOfSoundUsed;       }
    virtual double   range_resolution()    const { return this->metadata().rangeResolution;         }
    virtual double   temperature()         const { return this->metadata().temperature;             }
    virtual double   pressure()            const { return this->metadata().pressure;                }
};

class PingWrapper2 : public PingWrapper
{
    public:

    using Ptr      = std::shared_ptr<PingWrapper2>;
    using ConstPtr = std::shared_ptr<const PingWrapper2>;

    protected:

    PingWrapper2(const Message::ConstPtr& msg) :
        PingWrapper(msg)
    {
        if(msg->message_version() != 2) {
            throw std::runtime_error(
                "Tried to instanciate a PingWrapper2 with data from a PingWrapper1");
        }
    }

    public:

    static Ptr Create(const Message::ConstPtr& msg) { return Ptr(new PingWrapper2(msg)); }

    //virtual Ptr copy() const { return Create(this->msg_->copy()); } // not compiling
    virtual PingWrapper::Ptr copy() const { return Create(this->msg_->copy()); }

    const OculusSimplePingResult2& metadata() const {
        return *reinterpret_cast<const OculusSimplePingResult2*>(this->msg_->data().data());
    }

    virtual uint16_t       range_count()   const { return this->metadata().nRanges; }
    virtual uint16_t       bearing_count() const { return this->metadata().nBeams;  }
    virtual const int16_t* bearing_data()  const {
        return (const int16_t*)(this->data().data() + sizeof(this->metadata()));
    }
    virtual const uint8_t* ping_data() const {
        return this->data().data() + this->metadata().imageOffset;
    }
    virtual uint32_t ping_data_size() const { return this->metadata().imageSize; }

    //virtual bool    has_gains()   const { return this->metadata().fireMessage.flags | 0x4; } // is broken
    virtual bool has_gains() const {
        return this->metadata().imageSize > this->sample_size()*this->bearing_count()*this->range_count();
    }
    virtual uint8_t master_mode() const { return this->metadata().fireMessage.masterMode;  }
    virtual uint8_t sample_size() const {
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

    virtual uint32_t ping_index()        const { return this->metadata().pingId;                   }
    virtual uint32_t ping_firing_date()  const { return (uint32_t)this->metadata().pingStartTime;  }
    virtual double range()               const { return this->metadata().fireMessage.rangePercent; }
    virtual double gain_percent()        const { return this->metadata().fireMessage.gainPercent;  } 
    virtual double frequency()           const { return this->metadata().frequency;                }
    virtual double speed_of_sound_used() const { return this->metadata().speeedOfSoundUsed;        }
    virtual double range_resolution()    const { return this->metadata().rangeResolution;          }
    virtual double temperature()         const { return this->metadata().temperature;              }
    virtual double pressure()            const { return this->metadata().pressure;                 }
};

class PingMessage
{
    public:

    using Ptr      = std::shared_ptr<PingMessage>;
    using ConstPtr = std::shared_ptr<const PingMessage>;

    using TimeSource = Message::TimeSource;
    using TimePoint  = Message::TimePoint;

    static PingWrapper::Ptr make_ping_wrapper(const Message::ConstPtr& msg) {
        if(!msg || !msg->is_ping_message()) {
            return nullptr;
        }
        if(msg->message_version() == 2) {
            return PingWrapper2::Create(msg);
        }
        else {
            return PingWrapper1::Create(msg);
        }
    }

    protected:

    PingWrapper::ConstPtr pingData_;

    public: // for pybind11

    PingMessage(const Message::ConstPtr& msg) :
        pingData_(make_ping_wrapper(msg))
    {}

    public:

    static Ptr Create(const Message::ConstPtr& msg) { return Ptr(new PingMessage(msg)); }
    static Ptr Create(unsigned int size, const uint8_t* data, 
                      const TimePoint& stamp = TimePoint())
    {
        OculusMessageHeader header;
        std::memcpy(&header, data, sizeof(OculusMessageHeader));
        if(!is_ping_message(header) || size != header.payloadSize + sizeof(OculusMessageHeader)) {
            return nullptr;
        }
        return Create(Message::Create(size, data, stamp));
    }

    Message::ConstPtr           message()   const { return pingData_->message();   }
    const OculusMessageHeader&  header()    const { return pingData_->header();    }
    const std::vector<uint8_t>& data()      const { return pingData_->data();      }
    const TimePoint&            timestamp() const { return pingData_->timestamp(); }
    
    uint16_t       range_count()    const { return pingData_->range_count();   }
    uint16_t       bearing_count()  const { return pingData_->bearing_count(); }
    const int16_t* bearing_data()   const { return pingData_->bearing_data();  }
    const uint8_t* ping_data()      const { return pingData_->ping_data();     }
    uint32_t       step()           const { return pingData_->step();          }
    uint32_t       ping_data_size() const { return pingData_->ping_data_size();}
    uint32_t bearing_data_offset() const {
        return ((const uint8_t*)this->bearing_data()) - this->data().data();
    }
    uint32_t ping_data_offset() const {
        return this->ping_data() - this->data().data();
    }
    
    bool    has_gains()   const { return pingData_->has_gains();   }
    uint8_t master_mode() const { return pingData_->master_mode(); }
    uint8_t sample_size() const { return pingData_->sample_size(); }

    uint32_t ping_index()          const { return pingData_->ping_index();          }
    uint32_t ping_firing_date()    const { return pingData_->ping_firing_date();    }
    double   range()               const { return pingData_->range();               }
    double   gain_percent()        const { return pingData_->gain_percent();        }
    double   frequency()           const { return pingData_->frequency();           }
    double   speed_of_sound_used() const { return pingData_->speed_of_sound_used(); }
    double   range_resolution()    const { return pingData_->range_resolution();    }
    double   temperature()         const { return pingData_->temperature();         }
    double   pressure()            const { return pingData_->pressure();            }
};


} //namespace oculus

#endif //_DEF_OCULUS_DRIVER_OCULUS_MESSAGE_H_
