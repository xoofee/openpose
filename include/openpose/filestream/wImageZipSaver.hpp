#ifndef OPENPOSE_FILESTREAM_W_IMAGE_ZIP_SAVER_HPP
#define OPENPOSE_FILESTREAM_W_IMAGE_ZIP_SAVER_HPP

#include <memory> // std::shared_ptr
#include <string>
#include <openpose/thread/workerConsumer.hpp>
#include "imageSaver.hpp"
#include <queue> // std::priority_queue
#include <openpose/utilities/pointerContainer.hpp>
#include "iostream"
namespace op
{
    template<typename TDatums>
    class WImageZipSaver : public WorkerConsumer<TDatums>
    {
    public:
        explicit WImageZipSaver(const std::shared_ptr<ImageSaver>& imageSaver);

        void initializationOnThread();

        void workConsumer(const TDatums& tDatums);

    private:
        std::string lastPrefix;
        const std::shared_ptr<ImageSaver> spImageSaver;
        std::priority_queue<TDatums, std::vector<TDatums>, PointerContainerGreater<TDatums>> mPriorityQueueBuffer;
        DELETE_COPY(WImageZipSaver);
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
    WImageZipSaver<TDatums>::WImageZipSaver(const std::shared_ptr<ImageSaver>& imageSaver) :
        spImageSaver{imageSaver}
    {
    }

    template<typename TDatums>
    void WImageZipSaver<TDatums>::initializationOnThread()
    {
    }

    template<typename TDatums>
    void WImageZipSaver<TDatums>::workConsumer(const TDatums& tDatums)
    {
        try
        {
            if (checkNoNullNorEmpty(tDatums))
            {
                // Debugging log
                dLog("", Priority::Low, __LINE__, __FUNCTION__, __FILE__);
                // Profiling speed
                const auto profilerKey = Profiler::timerInit(__LINE__, __FUNCTION__, __FILE__);
                // T* to T
                // auto& tDatumsNoPtr = *tDatums;
                // Record image(s) on disk
                // std::vector<cv::Mat> cvOutputDatas(tDatumsNoPtr.size());
                // for (auto i = 0; i < tDatumsNoPtr.size(); i++)
                //     cvOutputDatas[i] = tDatumsNoPtr[i].cvOutputData;
                // const auto fileName = (!tDatumsNoPtr[0].name.empty() ? tDatumsNoPtr[0].name : std::to_string(tDatumsNoPtr[0].id));
                // spImageSaver->saveImages(cvOutputDatas, fileName);

                auto& tDatumsNoPtr = *tDatums;
                if(lastPrefix.empty() || lastPrefix == tDatumsNoPtr[0].prefix){
                    lastPrefix = tDatumsNoPtr[0].prefix;
                    mPriorityQueueBuffer.emplace(tDatums);
                }
                else if(lastPrefix != tDatumsNoPtr[0].prefix){

                    std::vector<cv::Mat> cvOutputDatas(mPriorityQueueBuffer.size());
                    std::vector<std::string> fileNames(mPriorityQueueBuffer.size());
                    int i = 0;

                    while (!mPriorityQueueBuffer.empty()){
                        
                        auto& tDatumsPtr = *mPriorityQueueBuffer.top();
                        cvOutputDatas[i] = tDatumsPtr[0].cvOutputData;
                        fileNames[i] = (!tDatumsPtr[0].name.empty() ? tDatumsPtr[0].name : std::to_string(tDatumsPtr[0].id));
                        i++;

                        // spImageSaver->saveImages(cvOutputDatas, fileName);

                        // std::cout<<"write:"<<fileName<<std::endl;
                        mPriorityQueueBuffer.pop();
                    }

                    spImageSaver->saveImagesZip(cvOutputDatas, fileNames);

                    lastPrefix = tDatumsNoPtr[0].prefix;
                    mPriorityQueueBuffer.emplace(tDatums);
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

    COMPILE_TEMPLATE_DATUM(WImageZipSaver);
}

#endif // OPENPOSE_FILESTREAM_W_IMAGE_ZIP_SAVER_HPP
