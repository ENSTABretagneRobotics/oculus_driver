#ifndef _DEF_OCULUS_DRIVER_RTAC_HELPERS_H_
#define _DEF_OCULUS_DRIVER_RTAC_HELPERS_H_

// This header is provided for convenience if you are using the rtac framework.
// It is not needed nor used by this driver.

#include <vector>

#include <oculus_driver/Oculus.h>

#include <rtac_base/types/Image.h>
#include <rtac_base/types/SonarPing2D.h>

namespace rtac { namespace types {

template <typename T, template<typename> class VectorT>
inline void oculus_to_rtac(SonarPing2D<T,VectorT>& dst, 
                           const OculusSimplePingResult& metadata,
                           const std::vector<uint8_t>& data)
{
    dst.resize({metadata.nBeams, metadata.nRanges});
    
    // copying bearing angles
    const uint16_t* bearingData = data.data() + sizeof(OculusSimplePingResult);
    // using intermediary std::vector in case VectorT is not directly
    // writable (such as rtac::cuda::DeviceVector)
    std::vector<float> bearings(metadata.nBeams);
    for(unsigned int i = 0; i < bearings.size(); i++) {
        bearings[i] = (0.01 * M_PI / 180.0) * bearingData[i];
    }
    dst.set_bearings(bearings);

    //copying ping data
    std::vector<T> pingData(dst.size());
    const uint8_t* data = data.data() + metadata.imageOffset;
    if(metadata.fireMessage.flags & 0x04) {
        // gain is sent
        for(unsigned int h = 0; h < dst.range_count(); h++) {
            float gain = 1.0f * sqrt((float)((const uint32_t*)data)[0]);
            data += 4;
            for(int w = 0; w < dst.bearing_count; w++) {
                pingData[dst.bearing_count()*w + h] = data[w];
            }
            data += dst.bearing_count();
        }
    }
    else {
        //gain was not sent
        for(int i = 0; i < pingData.size(); i++) {
            pingData[i] = data[i] / 255.0;
        }
    }
}

} //namespace types
} //namespace rtac

#endif //_DEF_OCULUS_DRIVER_RTAC_HELPERS_H_
