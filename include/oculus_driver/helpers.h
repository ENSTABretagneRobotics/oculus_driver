#ifndef _DEF_OCULUS_DRIVER_HELPERS_H_
#define _DEF_OCULUS_DRIVER_HELPERS_H_

#include <vector>
#include <cmath>
#include <fstream>
#include <string>

#include <oculus_driver/Oculus.h>

namespace oculus {

inline void write_pgm(const std::string& filename,
                      unsigned int width, unsigned int height,
                      const uint8_t* data)
{
    std::ofstream f(filename, std::ios::out | std::ios::binary);
    if(!f.is_open()) {
        std::ostringstream oss;
        oss << "Could not open file for writing : " << filename;
        throw std::runtime_error(oss.str());
    }
    
    f << "P5\n";
    f << width << " " << height << "\n" << 255 << "\n";
    f.write((const char*)data, width*height);

    f.close();
}

inline void write_pgm(const std::string& filename,
                      unsigned int width, unsigned int height,
                      const float* data)
{
    float m = data[0];
    float M = data[0];
    for(unsigned int i = 0; i < width*height; i++) {
        m = std::min(m, data[i]);
        M = std::max(M, data[i]);
    }

    float a = 255.0 / (M - m);
    float b = -m * a;
    std::vector<uint8_t> imageData(width*height);
    for(unsigned int i = 0; i < width*height; i++) {
        imageData[i] = a*data[i] + b;
    }
    write_pgm(filename, width, height, imageData.data());
}


/**
 * Getting some information about ping data in a more readable way.
 */
inline bool has_gains(const OculusSimpleFireMessage& msg)
{
    return msg.flags & 0x4;
}
inline bool has_gains(const OculusSimplePingResult& msg)
{
    return has_gains(msg.fireMessage);
}

inline bool has_16bits_data(const OculusSimpleFireMessage& msg)
{
    return msg.flags & 0x2;
}
inline bool has_16bits_data(const OculusSimplePingResult& msg)
{
    return has_16bits_data(msg.fireMessage);
}
inline double get_range(const OculusSimpleFireMessage& msg)
{
    return msg.range;
}
inline double get_range(const OculusSimplePingResult& msg)
{
    return get_range(msg.fireMessage);
}

inline bool has_gains(const OculusSimpleFireMessage2& msg)
{
    return msg.flags & 0x4;
}
inline bool has_gains(const OculusSimplePingResult2& msg)
{
    return has_gains(msg.fireMessage);
}

inline bool has_16bits_data(const OculusSimpleFireMessage2& msg)
{
    return msg.flags & 0x2;
}
inline bool has_16bits_data(const OculusSimplePingResult2& msg)
{
    return has_16bits_data(msg.fireMessage);
}
inline double get_range(const OculusSimpleFireMessage2& msg)
{
    return msg.rangePercent;
}
inline double get_range(const OculusSimplePingResult2& msg)
{
    return get_range(msg.fireMessage);
}

/**
 * This converts an OculusSimplePingResult to an array representing the data
 * after gain compensation (if those are present in the data).
 *
 * The dst data must have been previously reserved and must have at least
 * metadata.nBeams * metadata.nRanges elements.
 *
 * Data is encoded in row major format, a row being the set of bearing results
 * at a given range.
 *
 * OculusPingResultType is either a OculusSimplePingResult or an
 * OculusSimplePingResult2.
 */
template <typename T, class OculusPingResultType>
inline void ping_data_to_array(T* dst,
                               const OculusPingResultType& metadata,
                               const std::vector<uint8_t>& pingData)
{
    if(has_16bits_data(metadata)) {
        auto data = (const uint16_t*)(pingData.data() + metadata.imageOffset);
        if(has_gains(metadata)) {
            // gain is sent
            for(unsigned int h = 0; h < metadata.nRanges; h++) {
                float gain = 1.0f / sqrt((float)((const uint32_t*)data)[0]);
                data += 2;
                for(int w = 0; w < metadata.nBeams; w++) {
                    dst[metadata.nBeams*w + h] = gain * data[w];
                }
                data += metadata.nBeams;
            }
        }
        else {
            //gain was not sent
            for(unsigned int i = 0; i < metadata.nBeams*metadata.nRanges; i++) {
                dst[i] = data[i];
            }
        }
    }
    else {
        auto data = (const uint8_t*)(pingData.data() + metadata.imageOffset);
        if(has_gains(metadata)) {
            // gain is sent
            for(unsigned int h = 0; h < metadata.nRanges; h++) {
                float gain = 1.0f / sqrt((float)((const uint32_t*)data)[0]);
                data += 4;
                for(int w = 0; w < metadata.nBeams; w++) {
                    dst[metadata.nBeams*w + h] = gain * data[w];
                }
                data += metadata.nBeams;
            }
        }
        else {
            //gain was not sent
            for(unsigned int i = 0; i < metadata.nBeams*metadata.nRanges; i++) {
                dst[i] = data[i];
            }
        }
    }
}
inline std::vector<float> get_ping_acoustic_data(const std::vector<uint8_t>& pingData)
{
    auto header = *reinterpret_cast<const OculusMessageHeader*>(pingData.data());
    if(header.msgId != messageSimplePingResult) {
        throw std::runtime_error("Not a ping result");
    }
    if(header.msgVersion != 2) {
        auto metadata = *reinterpret_cast<const OculusSimplePingResult*>(pingData.data());
        std::vector<float> dst(metadata.nBeams * metadata.nRanges);
        ping_data_to_array(dst.data(), metadata, pingData);
        return dst;
    }
    else {
        auto metadata = *reinterpret_cast<const OculusSimplePingResult2*>(pingData.data());
        std::vector<float> dst(metadata.nBeams * metadata.nRanges);
        ping_data_to_array(dst.data(), metadata, pingData);
        return dst;
    }
}

/**
 * Returns a ping bearing information in radians
 */
template <typename T, class OculusPingResultType>
inline void get_ping_bearings(T* dst,
                              const OculusPingResultType& metadata,
                              const std::vector<uint8_t>& pingData)
{
    // copying bearing angles (
    auto bearingData = (const int16_t*)(pingData.data() + sizeof(OculusPingResultType));
    for(unsigned int i = 0; i < metadata.nBeams; i++) {
        dst[i] = (0.01 * M_PI / 180.0) * bearingData[i];
    }
}
inline std::vector<float> get_ping_bearings(const std::vector<uint8_t>& pingData)
{
    auto header = *reinterpret_cast<const OculusMessageHeader*>(pingData.data());
    if(header.msgId != messageSimplePingResult) {
        throw std::runtime_error("Not a ping result");
    }
    if(header.msgVersion != 2) {
        auto metadata = *reinterpret_cast<const OculusSimplePingResult*>(pingData.data());
        std::vector<float> dst(metadata.nBeams);
        get_ping_bearings(dst.data(), metadata, pingData);
        return dst;
    }
    else {
        auto metadata = *reinterpret_cast<const OculusSimplePingResult2*>(pingData.data());
        std::vector<float> dst(metadata.nBeams);
        get_ping_bearings(dst.data(), metadata, pingData);
        return dst;
    }
}

/**
 * Creates a cartesian image from the ping data.
 * 
 * This function is intended to be an example on how to interpret the data from
 * an OculusSimplePingResult. It is purposefully inefficient on maximize
 * readability. User are expected to implement their own version of this
 * function adapted to their own use case.
 *
 * This function return the image size as a std::pair(width,height).
 */
template <class OculusPingResultType>
inline std::pair<unsigned int, unsigned int> image_from_ping_data(
    const OculusPingResultType& metadata,
    const std::vector<uint8_t>& msgData,
    std::vector<float>& imageData,
    unsigned int imageWidth = 1024)
{
    std::vector<float> bearings = get_ping_bearings(msgData);
    std::vector<float> pingData = get_ping_acoustic_data(msgData);

    // Calculating the aspect ratio of the output image.
    float aperture = bearings.back() - bearings.front();
    float aspectRatio = 2*std::sin(0.5f*aperture);

    // calculating the output image size and reserving image data.
    unsigned int width  = imageWidth;
    unsigned int height = width / aspectRatio;
    imageData.resize(width*height);

    // The sonar data is not sampled linearly in the bearing dimension.
    // Making a lookup table to rapidly get bearing index from a bearing angle.
    // This comes down to performing a nearest neighbour interpolation.
    std::vector<unsigned int> bearingLut(metadata.nBeams);
    for(unsigned int b = 0; b < metadata.nBeams; b++) {
        float bearingAngle = ((bearings.back() - bearings.front())*b) / (metadata.nBeams - 1)
                           + bearings.front();
        unsigned int idx = 0;
        float minDiff = std::abs(bearings[0] - bearingAngle);
        for(int i = 1; i <bearings.size(); i++) {
            float diff = std::abs(bearings[i] - bearingAngle);
            if(diff < minDiff) {
                minDiff = diff;
                idx = i;
            }
            bearingLut[b] = idx;
        }
    }

    // Here we are filling the image data.  The x,y coordinates represent a
    // physical cartesian position relative to the sonar. x positive is forward
    // and y positive is right direction.
    // In the image coordinates, x points to the top and y points to the right.
    float imageResolution = get_range(metadata) / (height - 1);
    for(unsigned int h = 0; h < height; h++) {
        // inverting x dimension to have origin at the bottom a x up.
        float x = imageResolution * (height - 1 - h);
        for(unsigned int w = 0; w < width; w++) {
            float y = imageResolution * (w - 0.5f*width);
            float range   = sqrt(x*x + y*y);
            float bearing = std::atan2(y, x);
            if(range < 0.0f || range > get_range(metadata) || abs(bearing) > 0.5f*aperture) {
                // we are outside of the sonar fan
                imageData[width*h + w] = 0.0f;
            }
            else {
                // using the lookup table to compensate for the non-linearity
                // of the sampling in the bearing dimension.
                unsigned int bearingIndex = 
                    bearingLut[(metadata.nBeams - 1) * (bearing - bearings.front()) / aperture];    
                unsigned int rangeIndex = (metadata.nRanges - 1) * range / get_range(metadata);
                imageData[width*h + w] = pingData[metadata.nBeams*rangeIndex + bearingIndex];
            }
        }
    }

    return std::make_pair(width,height);
}

} //namespace oculus

#endif //_DEF_OCULUS_DRIVER_HELPERS_H_
