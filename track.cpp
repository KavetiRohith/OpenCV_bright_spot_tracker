#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

#define DEBUG 1

const int THRESHOLD = 50;

void absdiffCustom(const cv::Mat &src1, const cv::Mat &src2, cv::Mat &dst, pair<int, int> &centroid)
{
    // Check if the input matrices have the same size
    if (src1.size() != src2.size())
    {
        throw runtime_error("Input matrices must have the same size.");
    }

    // Allocate memory for the output matrix
    dst.create(src1.size(), src1.type());

    long long x_sum = 0;
    long long y_sum = 0;
    uint64 count = 0;

    // Iterate through each pixel and calculate the non-negative difference
    for (int i = 0; i < src1.rows; ++i)
    {
        for (int j = 0; j < src1.cols; ++j)
        {
            int difference = static_cast<int>(src1.at<uchar>(i, j)) - static_cast<int>(src2.at<uchar>(i, j));
            dst.at<uchar>(i, j) = static_cast<uchar>(std::max(0, difference));

            if (difference >= THRESHOLD)
            {
                y_sum += i;
                x_sum += j;
                count++;
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
void drawCrosshair(Mat &frame, const pair<int, int> &centroid, const pair<int, int> &prevCentroid)
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

        putText(frame, "No Movement detected using prev centroid", Point(50, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);

        // Draw horizontal line
        line(frame, Point(prevCentroid.first - 30, prevCentroid.second), Point(prevCentroid.first + 30, prevCentroid.second), Scalar(255, 255, 255), 2);

        // Draw vertical line
        line(frame, Point(prevCentroid.first, prevCentroid.second - 30), Point(prevCentroid.first, prevCentroid.second + 30), Scalar(255, 255, 255), 2);
    }
}

int main(int argc, char **argv)
{
    Mat mat_frame, mat_diff, mat_prev;
    std::vector<cv::Mat> channels;
    VideoCapture vcap;
    pair<int, int> centroid = make_pair(-1, -1);
    pair<int, int> prevCentroid = make_pair(-1, -1);

    // open the video stream and make sure it's opened
    //  "0" is the default video device which is normally the built-in webcam
    if (!vcap.open("Dark-Room-Laser-Spot-red-channel.mp4"))
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

    cv::split(mat_frame, channels);
    mat_frame = channels[0];

    mat_diff = mat_frame.clone();
    mat_prev = mat_frame.clone();

    namedWindow("Video", WINDOW_NORMAL);

    int frame_count = 1;
    int prev_reference_frame = 1;

    int frame_width = static_cast<int>(vcap.get(CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(vcap.get(CAP_PROP_FRAME_HEIGHT));

    Size frame_size(frame_width, frame_height);
    int fps = 30;

    VideoWriter video("output.mp4", VideoWriter::fourcc('m', 'p', '4', 'v'), fps, frame_size, false);

    while (1)
    {
        if (!vcap.read(mat_frame))
        {
            std::cout << "No frame" << std::endl;
            return 0;
        }

        frame_count++;

        cv::split(mat_frame, channels);
        mat_frame = channels[0];

        absdiffCustom(mat_frame, mat_prev, mat_diff, centroid);

        if (DEBUG && centroid.first == -1 && centroid.second == -1)
        {
            imwrite("DEBUG_IMGS/frame" + to_string(frame_count) + ".jpg", mat_frame);
            imwrite("DEBUG_IMGS/diff" + to_string(frame_count) + ".jpg", mat_diff);
        }

        drawCrosshair(mat_frame, centroid, prevCentroid);

        video.write(mat_frame);
        cv::imshow("Output Video", mat_frame);

        char c = (char)waitKey(1);
        if (c == 'q')
            break;

        // when threshold is not met, use the previous centroid for cases where there is no momvement of laser spot
        if (centroid.first != -1 && centroid.second != -1)
        {
            prevCentroid = centroid;
            // update the reference frame after 30 frames
            if (frame_count - prev_reference_frame > 30)
            {
                mat_prev = mat_frame.clone();
                prev_reference_frame = frame_count;
            }
        }
    }

    return 0;
}
