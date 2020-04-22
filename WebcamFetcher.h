#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>


class WebcamFetcher {
public:
    WebcamFetcher();

    void fetchFrameAndTransform();

    cv::Mat rawWebcamFrame;
    cv::Mat warpedFrame;
    cv::Mat tvImage;

    int fourByThreeFrames = 0;
    int twentyOneByNineFrames = 0;

private:
    void fetchFrame();
    void detectAndTrimBlackBoxes();

    cv::Mat perspectiveTransformMatrix;
    cv::VideoCapture cap;
};
