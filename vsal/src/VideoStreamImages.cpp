/************************************************************************************
*									   Includes										*
************************************************************************************/
#include "VideoStreamImages.h"
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

/************************************************************************************
*									  Namespaces									*
************************************************************************************/
using namespace boost::filesystem;

/************************************************************************************
*									 Declarations									*
************************************************************************************/
const std::string IMAGE_FILTER = 
	"(.*\\.(bmp|dib|jpeg|jpg|jpe|jp2|png|pbm|pgm|ppm|sr|ras))";

const std::string SPECIAL_CHARACTERS = ".[{}()*+?|^$";

/************************************************************************************
*									Implementation									*
************************************************************************************/
namespace vsal
{
    VideoStreamImages::VideoStreamImages(const std::string& path, double fps) :
		mPath(path),
        mWidth(0), mHeight(0),
        mCurrFrameIndex(0),
		mNextFrameIndex(0),
        mFPS(fps),
        mDt(1.0/fps)
    {
    }

    VideoStreamImages::~VideoStreamImages()
    {
        close();
    }

    bool VideoStreamImages::open()
    {
        close();
        getFrames(mPath, mFrames);
        if (mFrames.empty()) return false;

        // Read the first frame to extract meta data  
        mFrame = cv::imread(mFrames[0]);
        return true;
    }

    void VideoStreamImages::close()
    {
        if (!mFrame.empty()) mFrames.clear();
        mCurrFrameIndex = mNextFrameIndex = 0;
        mFrame.release();
    }

    bool VideoStreamImages::read()
    {
        if (mFrames.empty()) return false;
		if (mNextFrameIndex == mCurrFrameIndex)
		{
			++mNextFrameIndex;
			return true;
		}
		if (mNextFrameIndex >= mFrames.size()) return false;

		// Read next frame
		mFrame = cv::imread(mFrames[mNextFrameIndex]);
		mCurrFrameIndex = mNextFrameIndex;
		++mNextFrameIndex;
		return true;
    }

    int VideoStreamImages::getWidth() const
    {
        return mFrame.cols;
    }

    int VideoStreamImages::getHeight() const
    {
        return mFrame.rows;
    }

    double VideoStreamImages::getTimestamp() const
    {
        return mCurrFrameIndex*mDt;
    }

    double VideoStreamImages::getFPS() const
    {
        return mFPS;
    }

    void VideoStreamImages::getFrameData(unsigned char* data) const
    {
        memcpy(data, mFrame.data, mFrame.total() * mFrame.elemSize());
    }

	bool VideoStreamImages::isLive() const
	{
		return false;
	}

	bool VideoStreamImages::isOpened() const
	{
		return !mFrames.empty();
	}

    bool VideoStreamImages::isUpdated()
    {
        return true;
    }

    cv::Mat VideoStreamImages::getFrame()
    {
        return mFrame;
    }

    cv::Mat VideoStreamImages::getFrameGrayscale()
    {
        cv::Mat gray;
        cv::cvtColor(mFrame, gray, cv::COLOR_BGR2GRAY);
        return gray;
    }

    void VideoStreamImages::getFrames(const std::string& path,
        std::vector<std::string>& frames) const
    {
		if (is_directory(path)) getFramesFromDir(path, frames);
		else if (is_image(path)) frames.push_back(path);
		else if(is_valid_pattern(path)) getFramesFromPattern(path, frames);
    }

	void VideoStreamImages::getFramesFromDir(const std::string& dirPath,
		std::vector<std::string>& frames) const
	{
		boost::regex filter(IMAGE_FILTER);
		boost::smatch what;
		directory_iterator end_itr; // Default ctor yields past-the-end
		for (directory_iterator it(dirPath); it != end_itr; ++it)
		{
			// Skip if not a file
			if (!boost::filesystem::is_regular_file(it->status())) continue;

			// Get extension
			std::string ext = it->path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			// Skip if no match
			if (!boost::regex_match(ext, what, filter)) continue;

			frames.push_back(it->path().string());
		}
	}

	void VideoStreamImages::getFramesFromPattern(const std::string& path,
		std::vector<std::string>& frames) const
	{
		boost::filesystem::path fs_path(path);
        boost::filesystem::path dirPath = fs_path.parent_path();
        if(dirPath.empty()) dirPath = boost::filesystem::current_path();
		boost::regex filter(fs_path.filename().string());
		boost::smatch what;
		directory_iterator end_itr; // Default ctor yields past-the-end
		for (directory_iterator it(dirPath); it != end_itr; ++it)
		{
			// Skip if not a file
			if (!boost::filesystem::is_regular_file(it->status())) continue;

			// Skip if no match
			if (!boost::regex_match(it->path().filename().string(), what, filter)) continue;

			frames.push_back(it->path().string());
		}
	}

	size_t VideoStreamImages::getFrameIndex() const
	{
		return mCurrFrameIndex;
	}

	void VideoStreamImages::seek(size_t index)
	{
		mNextFrameIndex = index;
	}

	size_t VideoStreamImages::size() const
	{
		return mFrames.size();
	}

	bool VideoStreamImages::is_image(const std::string& path)
	{
		if (!is_regular_file(path)) return false;

		// Get extension
		boost::filesystem::path fs_path(path);
		std::string ext =  fs_path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		boost::regex filter(IMAGE_FILTER);
		boost::smatch what;
		return boost::regex_match(ext, what, filter);
	}

	bool VideoStreamImages::is_pattern(const std::string& path)
	{
		for (const char& c : SPECIAL_CHARACTERS)
			if (path.find(c) != std::string::npos) return true;
		return false;
	}

	bool VideoStreamImages::is_valid_pattern(const std::string& path)
	{
		if (path.empty()) return false;
		boost::filesystem::path fs_path(path);
		std::string parent_path = fs_path.parent_path().string();
		std::string filename = fs_path.filename().string();
		return (!is_pattern(parent_path) && is_pattern(filename));
	}

}	// namespace vsal