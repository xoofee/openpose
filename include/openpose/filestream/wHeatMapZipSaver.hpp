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
        std::priority_queue<TDatums, std::vector<TDatums>, PointerContainerGreater<TDatums>> mPriorityQueueBuffer;
        std::vector<Array<float>> poseHeatMapBuffer;
        std::vector<std::string> fileNamesBuffer;
        // int bufferCount;

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
                // // T* to T
                // auto& tDatumsNoPtr = *tDatums;
                // // Record image(s) on disk
                // std::vector<Array<float>> poseHeatMaps(tDatumsNoPtr.size());
                // for (auto i = 0; i < tDatumsNoPtr.size(); i++)
                //     poseHeatMaps[i] = tDatumsNoPtr[i].poseHeatMaps;
                // const auto fileName = (!tDatumsNoPtr[0].name.empty() ? tDatumsNoPtr[0].name : std::to_string(tDatumsNoPtr[0].id));
                // spHeatMapSaver->saveHeatMaps(poseHeatMaps, fileName);
                auto& tDatumsNoPtr = *tDatums;
                if(lastPrefix.empty() || lastPrefix == tDatumsNoPtr[0].prefix){
                    lastPrefix = tDatumsNoPtr[0].prefix;
                    auto& tDatumsPtr = *tDatums;
                    poseHeatMapBuffer.push_back(tDatumsPtr[0].poseHeatMaps);
                    fileNamesBuffer.push_back(!tDatumsPtr[0].name.empty() ? tDatumsPtr[0].name : std::to_string(tDatumsPtr[0].id));

                    // mPriorityQueueBuffer.emplace(tDatums);
                }
                else if(lastPrefix != tDatumsNoPtr[0].prefix){
                    spHeatMapSaver->saveHeatMapsZip(poseHeatMapBuffer, fileNamesBuffer);
                    poseHeatMapBuffer.clear();
                    fileNamesBuffer.clear();


                    lastPrefix = tDatumsNoPtr[0].prefix;
                    mPriorityQueueBuffer.emplace(tDatums);
                }
                // else if(lastPrefix != tDatumsNoPtr[0].prefix){
                //     // std::vector<cv::Mat> cvOutputDatas(mPriorityQueueBuffer.size());
                //     std::vector<Array<float>> poseHeatMaps(tDatumsNoPtr.size());
                //     std::vector<std::string> fileNames(mPriorityQueueBuffer.size());
                //     int i = 0;
                    
                //     while (!mPriorityQueueBuffer.empty()){
                //         int d=20;
                //         std::cout<<d++<<std::endl;
                //         auto& tDatumsPtr = *mPriorityQueueBuffer.top();
                //         std::cout<<d++<<std::endl;
                //         fileNames[i] = (!tDatumsPtr[0].name.empty() ? tDatumsPtr[0].name : std::to_string(tDatumsPtr[0].id));
                //         std::cout<<d++<<std::endl;
                //         poseHeatMaps[i] = tDatumsPtr[0].poseHeatMaps;//？
                //         i++;

                //         // spImageSaver->saveImages(cvOutputDatas, fileName);

                //         // std::cout<<"write:"<<fileName<<std::endl;
                //         std::cout<<d++<<std::endl;
                //         mPriorityQueueBuffer.pop();
                //         std::cout<<d++<<std::endl;
                //     }
                //     spHeatMapSaver->saveHeatMapsZip(poseHeatMaps, fileNames);

                //     lastPrefix = tDatumsNoPtr[0].prefix;
                //     mPriorityQueueBuffer.emplace(tDatums);
                // }





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
