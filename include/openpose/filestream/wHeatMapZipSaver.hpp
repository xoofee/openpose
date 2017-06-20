#ifndef OPENPOSE_FILESTREAM_W_HEAT_MAP_ZIP_SAVER_HPP
#define OPENPOSE_FILESTREAM_W_HEAT_MAP_ZIP_SAVER_HPP

#include <memory> // std::shared_ptr
#include <string>
#include <openpose/thread/workerConsumer.hpp>
#include "heatMapSaver.hpp"
#include <queue> // std::priority_queue
#include <openpose/utilities/pointerContainer.hpp>
#include "iostream"
namespace op
{
    template<typename TDatums>
    class WHeatMapZipSaver : public WorkerConsumer<TDatums>
    {
    public:
        explicit WHeatMapZipSaver(const std::shared_ptr<HeatMapSaver>& heatMapSaver);

        void initializationOnThread();

        void workConsumer(const TDatums& tDatums);

    private:
        std::vector<cv::Mat> poseHeatMapBuffer;
        std::vector<std::string> fileNamesBuffer;


        const std::shared_ptr<HeatMapSaver> spHeatMapSaver;
        std::string lastPrefix;
        DELETE_COPY(WHeatMapZipSaver);
    };
}





// Implementation
#include <vector>
#include <openpose/utilities/errorAndLog.hpp>
#include <openpose/utilities/macros.hpp>
#include <openpose/utilities/pointerContainer.hpp>
#include <openpose/utilities/profiler.hpp>
#include <openpose/utilities/openCv.hpp>
namespace op
{
    template<typename TDatums>
    WHeatMapZipSaver<TDatums>::WHeatMapZipSaver(const std::shared_ptr<HeatMapSaver>& heatMapSaver) :
        spHeatMapSaver{heatMapSaver}
    {
        poseHeatMapBuffer.reserve(500);
        fileNamesBuffer.reserve(500);

    }

    template<typename TDatums>
    void WHeatMapZipSaver<TDatums>::initializationOnThread()
    {
    }

    template<typename TDatums>
    void WHeatMapZipSaver<TDatums>::workConsumer(const TDatums& tDatums)
    {
        try
        {
            
            if (checkNoNullNorEmpty(tDatums))
            {
                
                // Debugging log
                dLog("", Priority::Low, __LINE__, __FUNCTION__, __FILE__);
                // Profiling speed
                const auto profilerKey = Profiler::timerInit(__LINE__, __FUNCTION__, __FILE__);

                auto& tDatumsNoPtr = *tDatums;
                if(lastPrefix.empty() || lastPrefix == tDatumsNoPtr[0].prefix){
                    lastPrefix = tDatumsNoPtr[0].prefix;
                    auto& tDatumsPtr = *tDatums;
                    cv::Mat heapmapImage;
                    unrollArrayToUCharCvMat(heapmapImage, tDatumsPtr[0].poseHeatMaps);
                    poseHeatMapBuffer.push_back(heapmapImage);
                    fileNamesBuffer.push_back(!tDatumsPtr[0].name.empty() ? tDatumsPtr[0].name : std::to_string(tDatumsPtr[0].id));
                }
                else if(lastPrefix != tDatumsNoPtr[0].prefix){
                    spHeatMapSaver->saveHeatMapsZip(poseHeatMapBuffer, fileNamesBuffer);
                    for(int i = 0; i<poseHeatMapBuffer.size();i++){
                       poseHeatMapBuffer[i].release();
                    }
                    poseHeatMapBuffer.clear();
                    fileNamesBuffer.clear();


                    lastPrefix = tDatumsNoPtr[0].prefix;
                }


                // Profiling speed
                Profiler::timerEnd(profilerKey);
                Profiler::printAveragedTimeMsOnIterationX(profilerKey, __LINE__, __FUNCTION__, __FILE__, Profiler::DEFAULT_X);
                // Debugging log
                dLog("", Priority::Low, __LINE__, __FUNCTION__, __FILE__);
            }
        }
        catch (const std::exception& e)
        {
            this->stop();
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }



    COMPILE_TEMPLATE_DATUM(WHeatMapZipSaver);
}

#endif // OPENPOSE_FILESTREAM_W_HEAT_MAP_ZIP_SAVER_HPP
