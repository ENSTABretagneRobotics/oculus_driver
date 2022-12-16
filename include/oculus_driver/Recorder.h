#ifndef _DEF_OCULUS_DRIVER_RECORDER_H_
#define _DEF_OCULUS_DRIVER_RECORDER_H_

#include <fstream>
#include <memory>
#include <chrono>
#include <sstream>

#include <oculus_driver/OculusMessage.h>

namespace oculus {

namespace blueprint {

// // these are taken as-is from Blueprint SDK
// struct RmLogHeader
// {
// public:
//   unsigned       fileHeader;   // Fixed 4 byte header ident type
//   unsigned       sizeHeader;   // Size of this structure
//   char           source[16];   // 12 character max source identifier
//   unsigned short version;      // File version type
//   unsigned short encryption;   // Encryption style (0 = none)
//   qint64         key;          // Possibly saved encryption key (0 otherwise)
//   double         time;         // Time of file creation
// };
//
// // ----------------------------------------------------------------------------
// // The post-ping fire message received back from the sonar ???
// struct RmLogItem
// {
// public:
//   unsigned       itemHeader;   // Fixed 4 byte header byte
//   unsigned       sizeHeader;   // Size of this structure
//   unsigned short type;         // Identifer for the contained data type
//   unsigned short version;      // Version for the data type
//   double         time;         // Time item creation
//   unsigned short compression;  // Compression type 0 = none, 1 = qCompress
//   unsigned       originalSize; // Size of the payload prior to any compression
//   unsigned       payloadSize;  // Size of the following payload
// };

// Replacing with fixed size types to avoid discrepancies between platforms.
// Unchecked alignment still an issue (probably)
struct LogHeader
{
    uint32_t fileHeader;   // Fixed 4 byte header ident type
    uint32_t sizeHeader;   // Size of this structure
    char     source[16];   // 12 character max source identifier
    uint16_t version;      // File version type
    uint16_t encryption;   // Encryption style (0 = none)
    int64_t  key;          // Possibly saved encryption key (0 otherwise)
    double   time;         // Time of file creation
};

struct LogItem
{
    uint32_t itemHeader;   // Fixed 4 byte header byte
    uint32_t sizeHeader;   // Size of this structure
    uint16_t type;         // Identifer for the contained data type
    uint16_t version;      // Version for the data type
    double   time;         // Time item creation
    uint16_t compression;  // Compression type 0 = none, 1 = qCompress
    uint32_t originalSize; // Size of the payload prior to any compression
    uint32_t payloadSize;  // Size of the following payload
};

enum RecordTypes
{
    rt_settings          = 1,    // RmSettingsLogger packet
    rt_serialPort        = 2,    // Raw serial string - version contains the port number
    rt_oculusSonar       = 10,   // Raw oculus sonar data
    rt_blueviewSonar     = 11,   // Blueview data log image (raw)
    rt_rawVideo          = 12,   // Raw video logg
    rt_h264Video         = 13,   // H264 compresses video log
    rt_apBattery         = 14,   // ApBattery structure
    rt_apMissionProgress = 15,   // ApMissionProgress structure
    rt_nortekDVL         = 16,   // The complete Nortek DVL structure
    rt_apNavData         = 17,   // ApNavData structures
    rt_apDvlData         = 18,   // ApDvlData structures
    rt_apAhrsData        = 19,   // ApAhrsData structure
    rt_apSonarHeader     = 20,   // ApSonarHeader followed by image
    rt_rawSonarImage     = 21,   // Raw sonar image
    rt_ahrsMtData2       = 22,   // XSens MtData2 message
    rt_apVehicleInfo     = 23,   // Artemis ApVehicleInfo structures
    rt_apMarker          = 24,   // ApMarker structure
    rt_apGeoImageHeader  = 25,   // ApGeoImageHeader
    rt_apGeoImageData    = 26,   // ApGeoImage data of image
    rt_sbgData           = 30,   // SBG compass data message
    rt_ocViewInfo        = 500,  // Oculus view information
    rt_oculusSonarStamp  = 1010
};

}

class Recorder
{
    public:

    static constexpr uint32_t    FileMagicNumber = 0x11223344;
    static constexpr uint32_t    ItemMagicNumber = 0xaabbccdd;
    static constexpr const char* SourceId        = "Oculus";

    // nanoseconds not necessary but ROS compatible
    struct TimeStamp {
        uint64_t seconds;
        uint64_t nanoseconds;

        static TimeStamp from_sonar_stamp(const Message::TimePoint& other) {
            return TimeStamp() = other;
        }

        Message::TimePoint to_sonar_stamp() const {
            uint64_t nanos = 1000000000*this->seconds + this->nanoseconds;
            return Message::TimePoint(std::chrono::nanoseconds(nanos));
        }

        TimeStamp& operator=(const Message::TimePoint& stamp) {
            this->nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
                stamp.time_since_epoch()).count();
            this->seconds      = this->nanoseconds / 1000000000;
            this->nanoseconds -= 1000000000*this->seconds;
            return *this;
        }

        template <typename T>
        T to_seconds() const { return (T)seconds + 1.0e-9*nanoseconds; }
    };

    protected:

    std::string           filename_;
    mutable std::ofstream file_;

    public:

    Recorder();
    ~Recorder();

    void open(const std::string& filename, bool force = false);
    void close();
    bool is_open() const { return file_.is_open(); }

    std::size_t write(const blueprint::LogItem& header,
                      const uint8_t* data) const;
    std::size_t write(const Message& message) const;
    std::size_t write(const Message::ConstPtr& message) const {
        return this->write(*message);
    }
};

class FileReader
{
    public:

    static constexpr uint32_t FileMagicNumber = Recorder::FileMagicNumber;
    static constexpr uint32_t ItemMagicNumber = Recorder::ItemMagicNumber;

    using TimeStamp = Recorder::TimeStamp;

    bool check_file_header(const blueprint::LogHeader& header);

    protected:

    std::string                filename_;
    mutable std::ifstream      file_;
    mutable blueprint::LogItem nextItem_;
    mutable std::size_t        itemPosition_;
    blueprint::LogHeader       fileHeader_;

    Message::Ptr message_;

    void read_next_header() const;

    public:

    FileReader(const std::string& filename);
    ~FileReader();

    void open(const std::string& filename);
    void close()         { file_.close(); }
    bool is_open() const { return file_.is_open(); }
    void rewind();

    const blueprint::LogHeader& file_header() const { return fileHeader_; }

    std::size_t current_item_position() const { return itemPosition_; }
    
    const blueprint::LogItem& next_item_header() const { return nextItem_; }
    std::size_t read_next_item(uint8_t* dst) const; // data is assumed to have been reserved
                                                    // using size given in next_item_header
    std::size_t jump_item() const;

    // These are for convenience
    std::size_t read_next_item(std::vector<uint8_t>& dst) const;

    Message::ConstPtr     read_next_message() const;
    PingMessage::ConstPtr read_next_ping()    const;
};

} // namespace oculus

#endif //_DEF_OCULUS_DRIVER_RECORDER_H_


