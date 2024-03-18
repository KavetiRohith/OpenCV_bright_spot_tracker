#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

// Function to compute the median of a vector
template <typename T>
T computeMedian(const std::vector<T> &values)
{
    auto values_copy = vector<T>(values);

    std::sort(values_copy.begin(), values_copy.end());

    return values_copy[values_copy.size() / 2];
}

cv::Mat computeBaseImage(const std::string &video_path)
{
    // Load images
    std::vector<cv::Mat> frames;

    // read frames from a video using opencv and store them in the vector
    VideoCapture cap(video_path);
    if (!cap.isOpened())
    {
        throw runtime_error("Error opening video file!");
    }

    vector<cv::Mat> channels;

    cv::Mat frame;

    int totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
    for (int i = 0; i < totalFrames; i += 100)
    {

        cap.set(cv::CAP_PROP_POS_FRAMES, i);
        cap.read(frame);
        cv::split(frame, channels);
        frame = channels[0];
        frames.push_back(frame.clone());
        // cv::imwrite("frame-" + std::to_string(i) + ".jpg", frame);
    }

    if (frames.empty())
    {
        throw runtime_error("Input vector is empty!");
        return cv::Mat();
    }

    int rows = frames[0].rows;
    int cols = frames[0].cols;

    // Output matrix for the result
    cv::Mat result(rows, cols, frames[0].type());

    // Iterate over each pixel
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            // Collect pixel values across all frames
            std::vector<int> pixelValues;
            for (const auto &frame : frames)
            {
                pixelValues.push_back(frame.at<uchar>(i, j)); // Assuming uchar images
            }

            // Compute the median and set the result
            result.at<uchar>(i, j) = computeMedian(pixelValues);
        }
    }

    return result;
}

int main()
{
    auto result = computeBaseImage("Dark-Room-Laser-Spot-red-channel.mp4");
    cv::imwrite("Dark-Room-Laser-Spot-red-channel-baseimage.jpg", result);

    result = computeBaseImage("Dark-Room-Laser-Spot-with-Clutter-red-channel.mp4");
    cv::imwrite("Dark-Room-Laser-Spot-with-Clutter-red-channel-baseimage.jpg", result);

    result = computeBaseImage("Light-Room-Laser-Spot-with-Clutter-red-channel.mp4");
    cv::imwrite("Light-Room-Laser-Spot-with-Clutter-red-channel-baseimage.jpg", result);
    return 0;
}
