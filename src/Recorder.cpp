#include <oculus_driver/Recorder.h>

#include <chrono>

namespace oculus {

Recorder::Recorder()
{}

Recorder::~Recorder()
{
    if(this->is_open()) {
        this->close();
    }
}

void Recorder::open(const std::string& filename, bool force)
{
    file_.open(filename, std::ofstream::out);
    if(!file_.is_open()) {
        std::ostringstream oss;
        oss << "Could not open file for writing : " << filename;
        throw std::runtime_error(oss.str());
    }

    // preparing file header
    blueprint::LogHeader header;
    std::memset(&header, 0, sizeof(header));

    header.fileHeader = FileMagicNumber;
    header.sizeHeader = sizeof(header);
    std::memcpy(header.source, SourceId, strlen(SourceId) + 1);
    header.version    = 1;
    header.encryption = 0;
    header.time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
    
    file_.write((const char*)&header, sizeof(header));
}

void Recorder::close()
{
    file_.close();
}

std::size_t Recorder::write(const OculusMessageHeader& header,
                            const std::vector<uint8_t>& data,
                            const SonarDriver::TimePoint& timestamp) const
{
    if(!this->is_open()) {
        return 0;
    }

    TimeStamp stamp;
    stamp = timestamp;
    
    // Writing oculus data
    blueprint::LogItem item;
    std::memset(&item, 0, sizeof(item));

    item.itemHeader   = ItemMagicNumber;
    item.sizeHeader   = sizeof(item);
    item.type         = blueprint::rt_oculusSonar;
    item.version      = 0;
    item.time         = stamp.to_seconds<double>();
    item.compression  = 0;
    item.originalSize = sizeof(header) + header.payloadSize;
    item.payloadSize  = item.originalSize;

    file_.write((const char*)&item, sizeof(item));
    file_.write((const char*)data.data(), item.payloadSize);
    std::size_t writtenSize = sizeof(item) + item.payloadSize;

    // Writing timestamp (not a Blueprint format)
    item.type         = blueprint::rt_oculusSonarStamp;
    item.originalSize = sizeof(TimeStamp);
    item.payloadSize  = item.originalSize;
    file_.write((const char*)&item,  sizeof(item));
    file_.write((const char*)&stamp, sizeof(stamp));
    writtenSize += sizeof(item) + item.payloadSize;

    return writtenSize;
}

} //namespace oculus

