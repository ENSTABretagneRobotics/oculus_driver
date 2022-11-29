#ifndef _DEF_OCULUS_PYTHON_OCULUS_MESSAGE_H_
#define _DEF_OCULUS_PYTHON_OCULUS_MESSAGE_H_

struct PyPingMessage
{
    protected:

    oculus::PingMessage::ConstPtr msg_;

    public:

    PyPingMessage(const oculus::PingMessage::ConstPtr& msg = nullptr) : msg_(msg) {}

    PyPingMessage copy() const { return PyPingMessage(msg_->copy()); }

    const OculusMessageHeader&  header()    const { return msg_->header();    }
    const std::vector<uint8_t>& data()      const { return msg_->data();      }
    //const TimePoint&            timestamp() const { return msg_->timestamp(); }
    
    uint16_t       range_count()   const { return msg_->range_count();   }
    uint16_t       bearing_count() const { return msg_->bearing_count(); }
    const int16_t* bearing_data()  const { return msg_->bearing_data();  }
    const uint8_t* ping_data()     const { return msg_->ping_data();     }
    
    bool    has_gains()   const { return msg_->has_gains();   }
    uint8_t master_mode() const { return msg_->master_mode(); }
    uint8_t sample_size() const { return msg_->sample_size(); }
};

void init_oculus_message(py::module& m);

#endif //_DEF_OCULUS_PYTHON_OCULUS_FILES_H_
