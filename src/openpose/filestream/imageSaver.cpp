#include <openpose/utilities/errorAndLog.hpp>
#include <openpose/filestream/fileStream.hpp>
#include <openpose/filestream/imageSaver.hpp>
#include "zip.h"
#include <iostream>
namespace op
{
    ImageSaver::ImageSaver(const std::string& directoryPath, const std::string& imageFormat) :
        FileSaver{directoryPath},
        mImageFormat{imageFormat}
    {
        try
        {
            if (mImageFormat.empty())
                error("The string imageFormat should not be empty.", __LINE__, __FUNCTION__, __FILE__);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    void ImageSaver::saveImages(const std::vector<cv::Mat>& cvOutputDatas, const std::string& fileName) const
    {
        try
        {
            // Record cv::mat
            if (!cvOutputDatas.empty())
            {
                // File path (no extension)
                const auto fileNameNoExtension = getNextFileName(fileName) + "_rendered";

                // Get names for each image
                std::vector<std::string> fileNames(cvOutputDatas.size());
                for (auto i = 0; i < fileNames.size(); i++)
                    fileNames[i] = {fileNameNoExtension + (i != 0 ? "_" + std::to_string(i) : "") + "." + mImageFormat};

                // Save each image
                for (auto i = 0; i < cvOutputDatas.size(); i++)
                    saveImage(cvOutputDatas[i], fileNames[i]);
            }
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }


    void ImageSaver::saveImagesZip(const std::vector<cv::Mat>& cvOutputDatas, const std::vector<std::string>& fileNamesTmp, std::string zipName) const
    {
        try
        {
            // Record cv::mat
            if (!cvOutputDatas.empty())
            {
                // Get names for each image
                std::vector<std::string> fileNames(cvOutputDatas.size());

                for (auto i = 0; i < fileNames.size(); i++){
                    const auto fileNameNoExtension = getNextFileName(fileNamesTmp[i]) + "_rendered";
                    fileNames[i] = {fileNameNoExtension + "." + mImageFormat};
                }
                    
                std::string fullFilePath = fileNames[0];
                int sep_index = mkdirForFile(fullFilePath);
                std::string folderPath = fullFilePath.substr(0,sep_index);
                std::string fileName = fullFilePath.substr(sep_index+1,fullFilePath.size());
                std::string zipPath = folderPath + "/" + zipName + ".zip";
                std::string extention = fileName.substr(fileName.rfind("."));
                int err =0; 

                struct zip* archive = zip_open(zipPath.c_str(), ZIP_CREATE||ZIP_TRUNCATE, &err);
                struct zip_source *src_ptr;
                std::vector<uchar> buf;

                
                
                // Save each image
                for (auto i = 0; i < cvOutputDatas.size(); i++){

                    fullFilePath = fileNames[i];
                    sep_index = fullFilePath.rfind("/");
                    folderPath = fullFilePath.substr(0,sep_index);
                    fileName = fullFilePath.substr(sep_index+1,fullFilePath.size());
                    zipPath = folderPath + "/" + zipName + ".zip";
                    extention = fileName.substr(fileName.rfind("."));

                    imencode(extention, cvOutputDatas[i], buf);
                    if((src_ptr = zip_source_buffer(archive, buf.data(), buf.size(), 0))==NULL||
                        zip_file_add(archive, fileName.c_str(), src_ptr,ZIP_FL_ENC_UTF_8)<0){
                        zip_source_free(src_ptr);
                        error("Zip file can not add " + fullFilePath + ".", __LINE__, __FUNCTION__, __FILE__);
                    }
                    std::cout<<"write:"<<fileName<<std::endl;
                }

                zip_close(archive);

            }
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
}
