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
    cap.set(cv::VideoCaptureProperties::CAP_PROP_EXPOSURE, 0.085);
    // disable auto-white balance
    cap.set(cv::VideoCaptureProperties::CAP_PROP_AUTO_WB, 0.0);

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
    warpedFrame = cv::Mat(INTERMEDIATE_HEIGHT, INTERMEDIATE_WIDTH, frame.type());
    tvImage = cv::Mat(INTERMEDIATE_HEIGHT, INTERMEDIATE_WIDTH, frame.type());
}

void WebcamFetcher::fetchFrame() {
    cap.read(rawWebcamFrame);
    if (rawWebcamFrame.empty()) {
        std::cerr << "ERROR! blank frame grabbed\n";
        exit(2);
    }
}

// a little more conservative than they need to be, we'd rather overcrop than under.
int constexpr BLACK_BAR_WIDTH = INTERMEDIATE_WIDTH / 7;
int constexpr BLACK_BAR_HEIGHT = INTERMEDIATE_HEIGHT / 8;
// Sample the middle right area to check for 4:3 images
// and middle top to check for 21:9 images
void WebcamFetcher::detectAndTrimBlackBoxes() {
    cv::Mat rightSideSample = warpedFrame
        .rowRange(INTERMEDIATE_HEIGHT / 2, (INTERMEDIATE_HEIGHT / 2) + 5)
        .colRange(INTERMEDIATE_WIDTH - (BLACK_BAR_WIDTH / 2), INTERMEDIATE_WIDTH - (BLACK_BAR_WIDTH / 2) + 5);
    cv::Scalar rightMean = cv::mean(rightSideSample);

    if (rightMean[0] < 20 && rightMean[1] < 20 && rightMean[2] < 20) {
        fourByThreeFrames++;
    }
    else {
        fourByThreeFrames = 0;
    }

    cv::Mat topMiddleSample = warpedFrame
        .rowRange(BLACK_BAR_HEIGHT / 2, (BLACK_BAR_HEIGHT / 2) + 5)
        .colRange(INTERMEDIATE_WIDTH / 2, (INTERMEDIATE_WIDTH / 2) + 5);
    cv::Scalar topMiddleMean = cv::mean(topMiddleSample);

    if (topMiddleMean[0] < 20 && topMiddleMean[1] < 20 && topMiddleMean[2] < 20) {
        twentyOneByNineFrames++;
    }
    else {
        twentyOneByNineFrames = 0;
    }

    if (fourByThreeFrames > 200) {
        tvImage = warpedFrame
            .rowRange(0, INTERMEDIATE_HEIGHT)
            .colRange(BLACK_BAR_WIDTH, INTERMEDIATE_WIDTH - BLACK_BAR_WIDTH);
    } else if (twentyOneByNineFrames > 200) {
        tvImage = warpedFrame
            .rowRange(BLACK_BAR_HEIGHT, INTERMEDIATE_HEIGHT - BLACK_BAR_HEIGHT)
            .colRange(0, INTERMEDIATE_WIDTH);
    }
    else {
        tvImage = warpedFrame;
    }
}

void WebcamFetcher::fetchFrameAndTransform() {
    this->fetchFrame();
    cv::warpPerspective(rawWebcamFrame, warpedFrame,
                        perspectiveTransformMatrix, warpedFrame.size());

    this->detectAndTrimBlackBoxes();
}
