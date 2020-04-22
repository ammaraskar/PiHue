#include "WebcamFetcher.h"
#include <iostream>

#define WIDTH 1024
#define HEIGHT 576

#define INTERMEDIATE_WIDTH 256
#define INTERMEDIATE_HEIGHT 144


WebcamFetcher::WebcamFetcher() {
    cv::Mat frame;
    int deviceID = 0;
    int apiID = cv::CAP_ANY;
    cap.open(deviceID + apiID);
    if (!cap.isOpened()) {
        std::cerr << "ERROR! Unable to open camera\n";
        exit(2);
    }
    cap.set(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH, WIDTH);
    cap.set(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT, HEIGHT);
    // disable auto exposure
    cap.set(cv::VideoCaptureProperties::CAP_PROP_AUTO_EXPOSURE, 0.25);
    cap.set(cv::VideoCaptureProperties::CAP_PROP_EXPOSURE, 0.095);
    cap.set(cv::VideoCaptureProperties::CAP_PROP_SATURATION, 0.55);

    cap.read(frame);
    frame.release();

    // Initialize the transformation matrices
    std::vector<cv::Point2f> transformationFrom{
        cv::Point2f(320, 217), cv::Point2f(523, 214),
        cv::Point2f(319, 352), cv::Point2f(520, 375)
    };
    std::vector<cv::Point2f> transformationTo{
        cv::Point2f(0, 0), cv::Point2f(INTERMEDIATE_WIDTH, 0),
        cv::Point2f(0, INTERMEDIATE_HEIGHT), cv::Point2f(INTERMEDIATE_WIDTH, INTERMEDIATE_HEIGHT)
    };
    perspectiveTransformMatrix = cv::getPerspectiveTransform(transformationFrom, transformationTo);

    // Initialize the cv::Mat objects needed to store the webcam frame
    // and computed image
    cap.read(rawWebcamFrame);
    tvImage = cv::Mat(INTERMEDIATE_HEIGHT, INTERMEDIATE_WIDTH, frame.type());
}

void WebcamFetcher::fetchFrame() {
    cap.read(rawWebcamFrame);
    if (rawWebcamFrame.empty()) {
        std::cerr << "ERROR! blank frame grabbed\n";
        exit(2);
    }
}

void WebcamFetcher::fetchFrameAndTransform() {
    this->fetchFrame();
    cv::warpPerspective(rawWebcamFrame, tvImage,
                        perspectiveTransformMatrix, tvImage.size());
}
