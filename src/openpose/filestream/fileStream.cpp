#include <opencv2/highgui/highgui.hpp> // cv::imread
#include <openpose/utilities/errorAndLog.hpp>
#include <openpose/filestream/jsonOfstream.hpp>
#include <openpose/filestream/fileStream.hpp>
#include "iostream"
#include "zip.h"

namespace op
{
    // Private class (on *.cpp)
    const auto errorMessage = "Json format only implemented in OpenCV for versions >= 3.0. Check savePoseJson instead.";

    std::string dataFormatToString(const DataFormat format)
    {
        try
        {
            if (format == DataFormat::Json)
                return "json";
            else if (format == DataFormat::Xml)
                return "xml";
            else if (format == DataFormat::Yaml)
                return "yaml";
            else if (format == DataFormat::Yml)
                return "yml";
            else
            {
                error("Undefined DataFormat.", __LINE__, __FUNCTION__, __FILE__);
                return "";
            }
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return "";
        }
    }

    std::string getFullName(const std::string& fileNameNoExtension, const DataFormat format)
    {
        return fileNameNoExtension + "." + dataFormatToString(format);
    }





    // Public classes (on *.hpp)
    DataFormat stringToDataFormat(const std::string& dataFormat)
    {
        try
        {
            if (dataFormat == "json")
                return DataFormat::Json;
            else if (dataFormat == "xml")
                return DataFormat::Xml;
            else if (dataFormat == "yaml")
                return DataFormat::Yaml;
            else if (dataFormat == "yml")
                return DataFormat::Yml;
            else
            {
                error("String does not correspond to any format (json, xml, yaml, yml)", __LINE__, __FUNCTION__, __FILE__);
                return DataFormat::Json;
            }
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return DataFormat::Json;
        }
    }

    void saveData(const std::vector<cv::Mat>& cvMats, const std::vector<std::string>& cvMatNames, const std::string& fileNameNoExtension, const DataFormat format)
    {
        
        try
        {
            // std::cout<<fileNameNoExtension.c_str()<<std::endl;
            if (format == DataFormat::Json && CV_MAJOR_VERSION < 3)
                error(errorMessage, __LINE__, __FUNCTION__, __FILE__);
            if (cvMats.size() != cvMatNames.size())
                error("cvMats.size() != cvMatNames.size()", __LINE__, __FUNCTION__, __FILE__);
            cv::FileStorage fileStorage{getFullName(fileNameNoExtension, format), cv::FileStorage::WRITE};
            mkdirForFile(getFullName(fileNameNoExtension, format));
            for (auto i = 0 ; i < cvMats.size() ; i++)
                fileStorage << cvMatNames[i] << cvMats[i];
            fileStorage.release();
        }
        catch (const std::exception& e)
        {
            // dont do anything
            // error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    void saveData(const cv::Mat& cvMat, const std::string cvMatName, const std::string& fileNameNoExtension, const DataFormat format)
    {
        try
        {
            saveData(std::vector<cv::Mat>{cvMat}, std::vector<std::string>{cvMatName}, fileNameNoExtension, format);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    std::vector<cv::Mat> loadData(const std::vector<std::string>& cvMatNames, const std::string& fileNameNoExtension, const DataFormat format)
    {
        try
        {
            if (format == DataFormat::Json && CV_MAJOR_VERSION < 3)
                error(errorMessage, __LINE__, __FUNCTION__, __FILE__);

            cv::FileStorage fileStorage{getFullName(fileNameNoExtension, format), cv::FileStorage::READ};
            std::vector<cv::Mat> cvMats(cvMatNames.size());
            for (auto i = 0 ; i < cvMats.size() ; i++)
                fileStorage[cvMatNames[i]] >> cvMats[i];
            fileStorage.release();
            return cvMats;
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return {};
        }
    }

    cv::Mat loadData(const std::string& cvMatName, const std::string& fileNameNoExtension, const DataFormat format)
    {
        try
        {
            return loadData(std::vector<std::string>{cvMatName}, fileNameNoExtension, format)[0];
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return {};
        }
    }

    void savePoseJson(const Array<float>& pose, const std::string& fileName, const bool humanReadable)
    {
        try
        {
            if (!pose.empty() && pose.getNumberDimensions() != 3)
                error("pose.getNumberDimensions() != 3.", __LINE__, __FUNCTION__, __FILE__);

            const auto numberPeople = pose.getSize(0);
            const auto numberBodyParts = pose.getSize(1);

            // Record frame on desired path
            JsonOfstream jsonOfstream{fileName, humanReadable};
            jsonOfstream.objectOpen();

            // Version
            jsonOfstream.key("version");
            jsonOfstream.plainText("0.1");
            jsonOfstream.comma();

            // Bodies
            jsonOfstream.key("people");
            jsonOfstream.arrayOpen();
            for (auto person = 0 ; person < numberPeople ; person++)
            {
                jsonOfstream.objectOpen();
                jsonOfstream.key("body_parts");
                jsonOfstream.arrayOpen();
                // Body parts
                for (auto bodyPart = 0 ; bodyPart < numberBodyParts ; bodyPart++)
                {
                    const auto finalIndex = 3*(person*numberBodyParts + bodyPart);
                    jsonOfstream.plainText(pose[finalIndex]);
                    jsonOfstream.comma();
                    jsonOfstream.plainText(pose[finalIndex+1]);
                    jsonOfstream.comma();
                    jsonOfstream.plainText(pose[finalIndex+2]);
                    if (bodyPart < numberBodyParts-1)
                        jsonOfstream.comma();
                }
                jsonOfstream.arrayClose();
                jsonOfstream.objectClose();
                if (person < numberPeople-1)
                {
                    jsonOfstream.comma();
                    jsonOfstream.enter();
                }
            }
            jsonOfstream.arrayClose();

            jsonOfstream.objectClose();
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    void saveImageZip(const cv::Mat& cvMat, const std::string& fullFilePath, std::string zipName, const std::vector<int>& openCvCompressionParams)
    {
        try
        {
            int sep_index = mkdirForFile(fullFilePath);
            std::string folderPath = fullFilePath.substr(0,sep_index);
            std::string fileName = fullFilePath.substr(sep_index+1,fullFilePath.size());
            std::string zipPath = folderPath + "/" + zipName + ".zip";
            std::string extention = fileName.substr(fileName.rfind("."));
            int err =0; 

            struct zip* archive = zip_open(zipPath.c_str(), ZIP_CREATE, &err);
            struct zip_source *src_ptr;
            std::vector<uchar> buf;
            imencode(extention, cvMat, buf);
            
            if((src_ptr = zip_source_buffer(archive, buf.data(), buf.size(), 0))==NULL||
            zip_file_add(archive, fileName.c_str(), src_ptr,ZIP_FL_ENC_UTF_8)<0){
                zip_source_free(src_ptr);
                error("Zip file can not add " + fullFilePath + ".", __LINE__, __FUNCTION__, __FILE__);
            }

            zip_close(archive);

            // if (!cv::imwrite(fullFilePath, cvMat, openCvCompressionParams))
            //     error("Image could not be saved on " + fullFilePath + ".", __LINE__, __FUNCTION__, __FILE__);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    void saveImage(const cv::Mat& cvMat, const std::string& fullFilePath, std::string zipName, const std::vector<int>& openCvCompressionParams)
    {
        try
        {
            int sep_index = mkdirForFile(fullFilePath);
            std::string folderPath = fullFilePath.substr(0,sep_index);
            std::string fileName = fullFilePath.substr(sep_index+1,fullFilePath.size());
            std::string zipPath = folderPath + "/" + zipName + ".zip";
            std::string extention = fileName.substr(fileName.rfind("."));
            int err =0; 

            struct zip* archive = zip_open(zipPath.c_str(), ZIP_CREATE, &err);
            struct zip_source *src_ptr;
            std::vector<uchar> buf;
            imencode(extention, cvMat, buf);
            
            if((src_ptr = zip_source_buffer(archive, buf.data(), buf.size(), 0))==NULL||
            zip_file_add(archive, fileName.c_str(), src_ptr,ZIP_FL_ENC_UTF_8)<0){
                zip_source_free(src_ptr);
                error("Zip file can not add " + fullFilePath + ".", __LINE__, __FUNCTION__, __FILE__);
            }

            zip_close(archive);

            // if (!cv::imwrite(fullFilePath, cvMat, openCvCompressionParams))
            //     error("Image could not be saved on " + fullFilePath + ".", __LINE__, __FUNCTION__, __FILE__);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    cv::Mat loadImage(const std::string& fullFilePath, const int openCvFlags)
    {
        try
        {
            cv::Mat cvMat = cv::imread(fullFilePath, openCvFlags);
            if (cvMat.empty())
                log("Empty image on path: " + fullFilePath + ".", Priority::Max, __LINE__, __FUNCTION__, __FILE__);
            return cvMat;
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return cv::Mat{};
        }
    }


    int mkdirForFile(std::string filePath)
    {
        int index = filePath.rfind("/");
        if(index<0){
            return index;
        } 
        else{
            mkdir(filePath.substr(0,index));
        }
        return index;
         
    }

    // void getZipName(std::string filePath)
    // {
    //     int index = filePath.rfind("/");
    //     if(index<0){
    //         return;
    //     } 
    //     else{
    //         mkdir(filePath.substr(0,index));
    //     }
    // }

}
