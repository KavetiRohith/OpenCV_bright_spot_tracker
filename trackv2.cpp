#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

#define DEBUG 1

const int THRESHOLD = 50;

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

        // we had to do this even though the video has been video encoded with single channel data
        // by setting the isColor property to false in the videowriter constructor
        // in q5 as the final encoded video still has three channels. however i have
        // verified that the same intensities for every pixel and are equivalent
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

void absdiffCustom(const cv::Mat &src1, const cv::Mat &src2, cv::Mat &dst, pair<int, int> &centroid)
{
    // Check if the input matrices have the same size
    if (src1.size() != src2.size())
    {
        std::cerr << "Error: Input matrices must have the same size." << std::endl;
        return;
    }

    // doesn't seem to make a difference hence commented out
    // GaussianBlur(src1, src1, Size(5, 5), 0, 0);
    // GaussianBlur(src2, src2, Size(5, 5), 0, 0);

    // Allocate memory for the output matrix
    dst.create(src1.size(), src1.type());

    long long x_sum = 0;
    long long y_sum = 0;
    long long count = 0;

    // Iterate through each pixel and calculate the non-negative difference
    for (int i = 0; i < src1.rows; ++i)
    {
        for (int j = 0; j < src1.cols; ++j)
        {
            int difference = static_cast<int>(src1.at<uchar>(i, j)) - static_cast<int>(src2.at<uchar>(i, j));
            dst.at<uchar>(i, j) = static_cast<uchar>(std::max(0, difference));

            if (difference >= THRESHOLD)
            {
                y_sum += i * difference;
                x_sum += j * difference;
                count += difference;
            }
        }
    }

    if (count > 0)
    {
        centroid.first = x_sum / (count);
        centroid.second = y_sum / (count);
    }
    else
    {
        centroid.first = -1;
        centroid.second = -1;
    }
}

// Function to draw a crosshair at specified centroid on the frame
void drawCrosshair(Mat &frame, const pair<int, int> &centroid)
{
    if (centroid.first != -1 || centroid.second != -1)
    {
        // Draw horizontal line
        line(frame, Point(centroid.first - 30, centroid.second), Point(centroid.first + 30, centroid.second), Scalar(255, 255, 255), 2);

        // Draw vertical line
        line(frame, Point(centroid.first, centroid.second - 30), Point(centroid.first, centroid.second + 30), Scalar(255, 255, 255), 2);
    }
    else
    {
        putText(frame, "No Laser detected", Point(50, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
    }
}

int main(int argc, char **argv)
{
    std::string video_name = "Dark-Room-Laser-Spot-blue-channel.mp4";
    // std::string video_name = "Dark-Room-Laser-Spot-with-Clutter-blue-channel.mp4";
    // std::string video_name = "Light-Room-Laser-Spot-with-Clutter-blue-channel.mp4";

    Mat mat_frame, mat_diff, mat_base;
    std::vector<cv::Mat> channels;
    VideoCapture vcap;
    pair<int, int> centroid = make_pair(-1, -1);

    mat_base = computeBaseImage(video_name);

    // open the video stream and make sure it's opened
    //  "0" is the default video device which is normally the built-in webcam
    if (!vcap.open(video_name))
    {
        std::cout << "Error opening video stream or file" << std::endl;
        return -1;
    }
    else
    {
        std::cout << "Opened video stream or file" << std::endl;
    }

    while (!vcap.read(mat_frame))
    {
        std::cout << "No frame" << std::endl;
        cv::waitKey(33);
    }

    // we had to do this even though the video has been video encoded with single channel data
    // by setting the isColor property to false in the videowriter constructor
    // in q5 as the final encoded video still has three channels. however i have
    // verified that the same intensities for every pixel and are equivalent
    cv::split(mat_frame, channels);
    mat_frame = channels[0];

    mat_diff = mat_frame.clone();

    namedWindow("Video", WINDOW_NORMAL);
    // namedWindow("Diff Video", WINDOW_NORMAL);

    int frame_count = 1;

    int frame_width = static_cast<int>(vcap.get(CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(vcap.get(CAP_PROP_FRAME_HEIGHT));

    Size frame_size(frame_width, frame_height);
    int fps = 30;

    VideoWriter video("output-threshold-" + to_string(THRESHOLD) + "-" + video_name, VideoWriter::fourcc('m', 'p', '4', 'v'), fps, frame_size, false);

    while (1)
    {
        if (!vcap.read(mat_frame))
        {
            std::cout << "No frame" << std::endl;
            return 0;
        }

        frame_count++;

        // we had to do this even though the video has been video encoded with single channel data
        // by setting the isColor property to false in the videowriter constructor
        // in q5 as the final encoded video still has three channels. however i have
        // verified that the same intensities for every pixel and are equivalent
        cv::split(mat_frame, channels);
        mat_frame = channels[0];

        absdiffCustom(mat_frame, mat_base, mat_diff, centroid);

        drawCrosshair(mat_frame, centroid);
        // drawCrosshair(mat_diff, centroid);

        if (DEBUG && centroid.first == -1 && centroid.second == -1)
        {
            imwrite("DEBUG_IMGS/frame" + to_string(frame_count) + ".jpg", mat_frame);
            imwrite("DEBUG_IMGS/diff" + to_string(frame_count) + ".jpg", mat_diff);
        }

        cv::imshow("Video", mat_frame);
        // cv::imshow("Diff Video", mat_diff);

        video.write(mat_frame);

        char c = (char)waitKey(1);
        if (c == 'q')
            break;
    }
}
