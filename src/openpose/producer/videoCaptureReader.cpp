#include <iostream>
#include <fstream>
#include <openpose/utilities/errorAndLog.hpp>
#include <openpose/utilities/fastMath.hpp>
#include <openpose/utilities/string.hpp>
#include <openpose/producer/videoCaptureReader.hpp>
#include <openpose/utilities/fileSystem.hpp>
namespace op
{
    
    VideoCaptureReader::VideoCaptureReader(const int index) :
        Producer{ProducerType::Webcam},
        mVideoCapture{index}
    {
        step = 5;
        try
        {
            // assert: make sure video capture was opened
            if (!isOpened())
                error("VideoCapture (webcam) could not be opened.", __LINE__, __FUNCTION__, __FILE__);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    VideoCaptureReader::VideoCaptureReader(const std::string& path) :
        Producer{ProducerType::Video}
    {
        step = 5;
        mVideoCounter = 0;
        if(path.substr(path.length()-3,3) == "txt"){
            std::ifstream infile(path.c_str());
            std::string line;
            while (std::getline(infile, line)) {
                videoPathList.push_back(line);
            }
        }
        else{
            videoPathList.push_back(path);
        }

        load();

        try
        {
            // assert: make sure video capture was opened
            if (!isOpened())
                error("VideoCapture (video) could not be opened for path: '" + path + "'.", __LINE__, __FUNCTION__, __FILE__);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }


    VideoCaptureReader::~VideoCaptureReader()
    {
        try
        {
            release();
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    std::string VideoCaptureReader::getFrameName()
    {
        try
        {
            const auto stringLength = 12u;
            int next_frame_index = get(CV_CAP_PROP_POS_FRAMES) == 0 ? 0 : get(CV_CAP_PROP_POS_FRAMES)+step-1;
            return toFixedLengthString( fastMax(0ll, longLongRound(next_frame_index)),   stringLength);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return "";
        }
    }
    std::string VideoCaptureReader::getFramePrefix()
    {
        try
        {
            return getFileNameNoExtension(videoPathList[mVideoCounter-1]);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return "";
        }
    }
    cv::Mat VideoCaptureReader::getRawFrame()
    {
        try
        {
            cv::Mat frame;
            // for (int i = 0; i<step; i++){   // throw (step-1) images
            //     if (!mVideoCapture.isOpened()){
            //         break;
            //     }
            //     mVideoCapture >> frame;
            // }
            if (get(CV_CAP_PROP_POS_FRAMES)==0){
                mVideoCapture >> frame;
            }
            else{
                for (int i=0;i<step;i++){
                    mVideoCapture >> frame;
                }
            }
            // std::cout<<get(CV_CAP_PROP_POS_FRAMES)<<std::endl;
            
            return frame;
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return cv::Mat{};
        }
    }

    cv::Mat VideoCaptureReader::getFrame()
    {
        try
        {
            cv::Mat frame;


            if (isOpened())
            {
                // If ProducerFpsMode::OriginalFps, then force producer to keep the frame rate of the frames producer sources (e.g. a video)
                keepDesiredFrameRate();
                // Get frame
                frame = getRawFrame();
                // Flip + rotate frame
                flipAndRotate(frame);
                // Check frame integrity
                checkFrameIntegrity(frame);
                // Check if video capture did finish and close/restart it
                ifEndedResetOrRelease();
            }
            return frame;
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return cv::Mat{};
        }
    }
    void VideoCaptureReader::ifEndedResetOrRelease()
    {
        try
        {
            if (isOpened())
            {
                // OpenCV closing issue: OpenCV goes in the range [1, get(CV_CAP_PROP_FRAME_COUNT) - 1] in some videos (i.e. there is a frame missing),
                // mNumberEmptyFrames allows the program to be properly closed keeping the 0-index frame counting
                int next_frame_index = get(CV_CAP_PROP_POS_FRAMES) == 0 ? 0 : get(CV_CAP_PROP_POS_FRAMES)+step-1;
                if (mNumberEmptyFrames > 2 || (mType != ProducerType::Webcam && next_frame_index >= get(CV_CAP_PROP_FRAME_COUNT)))
                {
                    // Repeat video
                    if (mProperties[(unsigned char)ProducerProperty::AutoRepeat])
                        set(CV_CAP_PROP_POS_FRAMES, 0);

                    // Warning + release mVideoCapture
                    else
                        release();
                    reset(mNumberEmptyFrames, mTrackingFps);
                }
            }
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }


    bool VideoCaptureReader::load()
    {
        if(videoPathList.size() == mVideoCounter){
            return false;
        }
        else{
            mVideoCapture.release ();
            printf("%s\n", ("Load: " + videoPathList[mVideoCounter]).c_str());
            mVideoCapture = cv::VideoCapture(videoPathList[mVideoCounter++]);
            
            return true;
        }
        return false;

    }

    bool VideoCaptureReader::isOpened()
    {
        if(mVideoCapture.isOpened()){
            return true;
        }
        else{
            return load();
        }
        
    }

    void VideoCaptureReader::release()
    {
        try
        {
            if (mVideoCapture.isOpened())
            {
                mVideoCapture.release();
                log("cv::VideoCapture released.", Priority::Low, __LINE__, __FUNCTION__, __FILE__);
            }
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    double VideoCaptureReader::get(const int capProperty)
    {
        try
        {
            // Specific cases
            // If rotated 90 or 270 degrees, then width and height is exchanged
            if ((capProperty == CV_CAP_PROP_FRAME_WIDTH || capProperty == CV_CAP_PROP_FRAME_HEIGHT) && (get(ProducerProperty::Rotation) != 0. && get(ProducerProperty::Rotation) != 180.))
            {
                if (capProperty == CV_CAP_PROP_FRAME_WIDTH)
                    return mVideoCapture.get(CV_CAP_PROP_FRAME_HEIGHT);
                else
                    return mVideoCapture.get(CV_CAP_PROP_FRAME_WIDTH);
            }

            // Generic cases
            return mVideoCapture.get(capProperty);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return 0.;
        }
    }
}
