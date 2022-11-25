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
    file_.open(filename, std::ofstream::binary);
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

FileReader::FileReader(const std::string& filename) :
    itemPosition_(0)
{
    std::memset(&nextItem_, 0, sizeof(nextItem_));
    this->open(filename);
}

FileReader::~FileReader()
{
    if(this->is_open()) {
        this->close();
    }
}

bool FileReader::check_file_header(const blueprint::LogHeader& header)
{
    if(header.fileHeader != FileMagicNumber) {
        std::ostringstream oss;
        oss << "oculus::FileReader : invalid file header, is it a .oculus file ?.\n";
        oss << "    file : '" << filename_ << "'";
        throw std::runtime_error(oss.str());
    }
    if(header.version != 1) {
        std::cerr << "oculus::FileHeader version is != 1. Reaingd may fail" << std::endl;
    }
    if(header.encryption != 0) {
        std::ostringstream oss;
        oss << "oculus::FileReader : file is encrypted. Cannot decode.\n";
        oss << "    file : '" << filename_ << "'";
        throw std::runtime_error(oss.str());
    }
    return true;
}

void FileReader::open(const std::string& filename)
{
    filename_ = filename;
    file_.open(filename, std::ifstream::binary);
    if(!file_.is_open()) {
        std::ostringstream oss;
        oss << "Could not open file for reading : " << filename;
        throw std::runtime_error(oss.str());
    }

    // reading and cheacking file header
    blueprint::LogHeader header;
    file_.read((char*)&header, sizeof(header));
    if(!file_) {
        std::ostringstream oss;
        oss << "oculus::FileReader : error reading file header.\n";
        oss << "    file : '" << filename_ << "'";
        throw std::runtime_error(oss.str());
    }
    FileReader::check_file_header(header);

    itemPosition_ = file_.tellg();
    file_.read((char*)&nextItem_, sizeof(nextItem_));
    if(!file_) {
        std::memset(&nextItem_, 0, sizeof(nextItem_));
        itemPosition_ = 0;
        std::ostringstream oss;
        oss << "oculus::FileReader : error reading first item. The file may be empty.\n";
        oss << "    file : '" << filename_ << "'";
        throw std::runtime_error(oss.str());
    }
}

std::size_t FileReader::jump_next() const
{
    if(nextItem_.type == 0) {
        return 0;
    }

    if(!file_.seekg(nextItem_.payloadSize, std::ios::cur)) {
        std::memset(&nextItem_, 0, sizeof(nextItem_));
        std::ostringstream oss;
        oss << "oculus::FileReader : error jumping to next item. File might be corrupted.\n";
        oss << "    file : '" << filename_ << "'";
        throw std::runtime_error(oss.str());
    }
    std::size_t currentItemPosition = itemPosition_;

    itemPosition_ = file_.tellg();
    file_.read((char*)&nextItem_, sizeof(nextItem_));
    if(!file_) {
        std::memset(&nextItem_, 0, sizeof(nextItem_));
        itemPosition_ = 0;
        if(!file_.eof()) {
            std::ostringstream oss;
            oss << "oculus::FileReader : error reading next item header. ";
            oss << "The file may be corrupted.\n";
            oss << "    file : '" << filename_ << "'";
            throw std::runtime_error(oss.str());
        }
    }
    return currentItemPosition;
}

std::size_t FileReader::read_next(blueprint::LogItem& header, std::vector<uint8_t>& data) const
{
    if(nextItem_.type == 0) {
        return 0;
    }

    // reading data chunk
    header = nextItem_;
    data.resize(header.payloadSize);
    if(!file_.read((char*)data.data(), header.payloadSize)) {
        std::memset(&nextItem_, 0, sizeof(nextItem_));
        std::ostringstream oss;
        oss << "oculus::FileReader : error reading item data. File might be corrupted.\n";
        oss << "    file : '" << filename_ << "'";
        throw std::runtime_error(oss.str());
    }
    std::size_t currentItemPosition = itemPosition_;

    itemPosition_ = file_.tellg();
    file_.read((char*)&nextItem_, sizeof(nextItem_));
    if(!file_) {
        std::memset(&nextItem_, 0, sizeof(nextItem_));
        itemPosition_ = 0;
        if(!file_.eof()) {
            std::ostringstream oss;
            oss << "oculus::FileReader : error reading next item header. ";
            oss << "The file may be corrupted.\n";
            oss << "    file : '" << filename_ << "'";
            throw std::runtime_error(oss.str());
        }
    }
    return currentItemPosition;
}

} //namespace oculus

