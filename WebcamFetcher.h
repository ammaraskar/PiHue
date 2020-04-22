#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>


class WebcamFetcher {
public:
    WebcamFetcher();

    void fetchFrameAndTransform();

    cv::Mat rawWebcamFrame;
    cv::Mat tvImage;
private:
    void fetchFrame();

    cv::Mat perspectiveTransformMatrix;
    cv::VideoCapture cap;
};
