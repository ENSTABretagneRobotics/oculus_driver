#include <oculus_driver/Recorder.h>
#include <oculus_driver/print_utils.h>

#include <iostream>

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

std::size_t Recorder::write(const blueprint::LogItem& header,
                            const uint8_t* data) const
{
    if(!this->is_open()) {
        return 0;
    }

    file_.write((const char*)&header, sizeof(header));
    file_.write((const char*)data, header.payloadSize);
    return sizeof(header) + header.payloadSize;
}

std::size_t Recorder::write(const Message& message) const
{
    if(!this->is_open()) {
        return 0;
    }
    std::size_t writtenSize = 0;

    TimeStamp stamp = TimeStamp::from_sonar_stamp(message.timestamp());

    blueprint::LogItem item;
    std::memset(&item, 0, sizeof(item));
    
    item.itemHeader   = ItemMagicNumber;
    item.sizeHeader   = sizeof(item);
    item.type         = blueprint::rt_oculusSonar;
    item.version      = 0;
    item.time         = stamp.to_seconds<double>();
    item.compression  = 0;
    item.originalSize = message.data().size();
    item.payloadSize  = item.originalSize;
    writtenSize += this->write(item, message.data().data());

    item.type         = blueprint::rt_oculusSonarStamp;
    item.originalSize = sizeof(stamp);
    item.payloadSize  = item.originalSize;
    writtenSize += this->write(item, (const uint8_t*)&stamp);

    return writtenSize;
}

FileReader::FileReader(const std::string& filename) :
    itemPosition_(0),
    message_(new Message())
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

    file_.read((char*)&fileHeader_, sizeof(fileHeader_));
    if(!file_) {
        std::ostringstream oss;
        oss << "oculus::FileReader : error reading file header.\n";
        oss << "    file : '" << filename_ << "'";
        throw std::runtime_error(oss.str());
    }
    FileReader::check_file_header(fileHeader_);
    this->read_next_header();
}

void FileReader::rewind()
{
    file_.seekg(sizeof(fileHeader_));
    this->read_next_header();
}

void FileReader::read_next_header() const
{
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
}

std::size_t FileReader::jump_item() const
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
    this->read_next_header();
    return currentItemPosition;
}

std::size_t FileReader::read_next_item(uint8_t* dst) const
{
    if(nextItem_.type == 0) {
        return 0;
    }

    if(!file_.read((char*)dst, nextItem_.payloadSize)) {
        std::memset(&nextItem_, 0, sizeof(nextItem_));
        std::ostringstream oss;
        oss << "oculus::FileReader : error reading item data. File might be corrupted.\n";
        oss << "    file : '" << filename_ << "'";
        throw std::runtime_error(oss.str());
    }

    std::size_t currentItemPosition = itemPosition_;
    this->read_next_header();
    return currentItemPosition;
}


std::size_t FileReader::read_next_item(std::vector<uint8_t>& dst) const
{
    if(nextItem_.type == 0) {
        return 0;
    }
    dst.resize(nextItem_.payloadSize);
    return this->read_next_item(dst.data());
}

Message::ConstPtr FileReader::read_next_message() const
{
    // Reading file until we find a rt_oculusSonar message or enf of file
    while(nextItem_.type != blueprint::rt_oculusSonar && this->jump_item());
    if(nextItem_.type == 0) {
        return nullptr;
    }

    double nextItemDate = nextItem_.time;
    this->read_next_item(message_->data_);
    (*message_).update_from_data();

    // Reading TimeStamp from next message if it is there. Falling back to the
    // LogItem date if it is not there.
    if(nextItem_.type == blueprint::rt_oculusSonarStamp) {
        // next message is timestamp associated with the message we just read.
        TimeStamp stamp;
        this->read_next_item((uint8_t*)&stamp);
        message_->timestamp_ = stamp.to_sonar_stamp();
    }
    else {
        uint64_t nanos = 1000000000*nextItemDate;
        message_->timestamp_ = Message::TimePoint(std::chrono::nanoseconds(nanos));
    }

    return message_;
}

PingMessage::ConstPtr FileReader::read_next_ping() const
{
    Message::ConstPtr msg = this->read_next_message();
    while(msg && !msg->is_ping_message()) {
        msg = this->read_next_message();
    }
    if(!msg)
        return nullptr;
    return PingMessage::Create(msg);
}

} //namespace oculus

