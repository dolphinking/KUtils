#pragma once

#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

#include "seq.hpp"
#include "mouse.hpp"

using namespace std;

/*! Human-related computer vision utilities. */
namespace humancv {
    /*! Since contours are actually "ring" structures but are represented as
     * `vector`s in OpenCV, we wrap them so that indices wrap around.
     */
    typedef seq::WrappedSeq<vector<cv::Point>> Contour;

    /*! Indices into a `Contour` denoting a finger. */
    struct FingerData
    {
        /*! Fingertip index. */
        size_t i;
        /*! Left index of contour representing edge of finger. */
        size_t leftI;
        /*! Right index of contour representing edge of finger. */
        size_t rightI;

        friend ostream &operator<<(ostream &out, const FingerData &f);
    };

    /*! Store finger data from a `Contour` representing a hand (with
     * possible arm attached).
     *
     * @param ctr
     *      The `Contour` to search for fingers in.
     * @param handSize
     *      Estimated hand size (must be big enough to contain any possible
     *      hands).
     * @param out
     *      Output vector of `FingerData` objects.
     * @param k
     *      Number to use for k-curvature computation.
     * @param minDist
     *      Minimum distance between finger peaks.
     * @param fingerAngle
     *      Maximum angle (curvature) a finger can have.
     *
     * See the implementation in `cfinder.cpp` for more details.
     */
    void getFingers(
            const Contour &ctr,
            const cv::Size &handSize,
            vector<FingerData> &out,
            int k,
            float minDist,
            float fingerAngle=1.0
            );

    /*! Basic face detection and modeling. */
    namespace face
    {
        /*! Given rectangles containing face and the size of the parent image,
         * store face skin masks in dest of size `imSize`.
         *
         * Faces are approximated as ellipses with a small overlapping rectangle
         * to model the neck.
         *
         * @param imSize
         *      Size of parent image where faces were found.
         * @param rois
         *      Vector of face rectangles.
         * @param dest
         *      Destination mask to store faces - will be resized to `imSize`.
         */
        void storeMasks(const cv::Size imSize, const vector<cv::Rect> &rois, cv::Mat_<uchar> &dest);

        /*! Store rectangles containing faces in `out`.
         *
         * @param im
         *      Grayscale image to search for faces.
         * @param cascade
         *      Classifier cascade, typically loaded from an XML file. OpenCV
         *      comes with several of these; look for the `haarcascades`
         *      directory in your OpenCV install path.
         * @param out
         *      Output vector of `cv::Rect`s.
         * @param scale
         *      Scaling factor for the image. Image is scaled by this factor
         *      before being passed into the face detector, which may improve
         *      performance for large images.
         */
        void getRects(
                const cv::Mat_<uchar> &im,
                cv::CascadeClassifier &cascade,
                vector<cv::Rect> &out,
                float scale=1
                );
    }

    /*! Color-based skin segmentation. */
    namespace skin
    {
        /*! Store a binary mask with skin pixels = 255, non-skin pixels = 0.
         *
         * Runs a boosted classifier on each pixel to classify it.
         *
         * @param im
         *      Image to compute the mask on. Must have same pixel format as
         *      the one used to train the classifier.
         * @param cls
         *      Boosted classifier that takes a 1x3 `Mat` representing one pixel
         *      and classifies it as skin/non-skin. It should have been trained
         *      with skin as the higher-valued class, as `thres` is the minimum
         *      response (weighted sum) required to classify a pixel as skin.
         * @param dest
         *      Where to store the resulting mask.
         * @param lookup
         *      Pointer to a lookup table for the pixels - can significantly
         *      speed up computation. If `NULL` is provided a new one will be
         *      initialized and destroyed each function call.
         * @param thres
         *      Pixels with classifier responses (weighted sums) above this
         *      threshold will be classified as skin.
         */
        void getMask(
                const cv::Mat_<cv::Vec3b> &im,
                const CvBoost &cls,
                cv::Mat_<uchar> &dest,
                cv::SparseMat_<float> *lookup=NULL,
                float thres=0
                );

    }

    /*! Framework for tracking hand gestures.
     *
     * It has two phases, training and runtime.
     *
     * ## Training
     *
     * A sequence of frames, each containing a face, is gathered from a video
     * source.
     *
     * This sequence is passed to `train()`, faces are detected, and an Adaboost
     * classifier is trained with the face pixels as the positive class and all
     * other pixels as the negative class. The assumption is that no other
     * objects containing skin, besides the faces, are present in the frames.
     *
     * ## Runtime
     *
     * <TODO>
     */
    class CursorFinder
    {
    private:
        // these are all temporary variables placed here so they don't have to be
        // initalized each time
        cv::Mat_<uchar> gray;
        cv::Mat_<cv::Vec3b> ycrcb;

        cv::Mat_<uchar> skinMask;
        vector<vector<cv::Point>> skinContours;

        vector<cv::Rect> faceRects;
    public:
        cv::CascadeClassifier cascade;
        float addChance;
        float minCtrProp;
        float handSizeProp;
        float kHandHeightProp;
        float minFingerDistProp;

        CvBoost boost;
        cv::SparseMat_<float> pxToPrediction;
        float thres;

        cv::KalmanFilter kFilter;

        cv::Rect screenRect;
        cv::Rect mouseRect;

        /*! @param mouseRect
         *      Rectangle in frame defining hand borders.
         * @param screenRect
         *      Rectangle defining the screen - where points in `mouseRect` get
         *      projected to.
         * @param cascade
         *      `cv::CascadeClassifier` for detecting faces.
         * @param addChance
         *      Probability that pixel gets added to training samples - used to
         *      reduce training time.
         * @param minCtrProp
         *      Minimum area of contour relative to face (as a proportion).
         * @param handSizeProp
         *      Estimated size of hand relative to face (as a proportion).
         * @param kHandHeightProp
         *      Multiplied by hand height to get k for k-curvature.
         * @param minFingerDistProp
         *      Multiplied by hand height to get minimum distance between
         *      fingers.
         */
        CursorFinder(
                const cv::Rect &mouseRect,
                const cv::Rect &screenRect,
                const cv::CascadeClassifier &cascade,
                float addChance=0.1,
                float minCtrProp=0.05,
                float handSizeProp=1,
                float kHandHeightProp=0.25,
                float minFingerDistProp=0.15
                );

        /*! <TODO> */
        void mouseStateFromContour(
                const Contour &ctr,
                const cv::Size &handSize,
                const cv::Size &windowSize,
                mouse::State &out
                ) const ;

        /*! <TODO> */
        void train(const vector<cv::Mat_<cv::Vec3b>> &negFrames);

        /*! Return true if new mouse state was stored in `out` (ie. hand was found).
         */
        bool getMouseState(
                const cv::Mat_<cv::Vec3b> &frame,
                mouse::State &out,
                cv::Mat_<uchar> *skinMask=NULL,
                cv::Mat_<uchar> *withoutFaceAndSmall=NULL,
                cv::Mat_<uchar> *withHighestCtr=NULL,
                bool *foundFace=NULL
                );
    };
}
