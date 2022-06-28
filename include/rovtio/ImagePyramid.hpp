/*
* Copyright (c) 2014, Autonomous Systems Lab
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of the Autonomous Systems Lab, ETH Zurich nor the
* names of its contributors may be used to endorse or promote products
* derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#ifndef IMAGEPYRAMID_HPP_
#define IMAGEPYRAMID_HPP_

#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "rovtio/FeatureCoordinates.hpp"

namespace rovtio{

/** \brief Halfsamples an image.
 *
 *   @param imgIn - Input image.
 *   @param imgOut - Output image (halfsampled).
 */
//void halfSample(const cv::Mat& imgIn,cv::Mat& imgOut){
//  std::cerr << "This should not be used.\n";
//  throw std::logic_error("Not implemented of floating points.\n");
//  imgOut.create(imgIn.rows/2,imgIn.cols/2,imgIn.type());
//  const int refStepIn = imgIn.step.p[0];
//  const int refStepOut = imgOut.step.p[0];
//  uint8_t* imgPtrInTop;
//  uint8_t* imgPtrInBot;
//  uint8_t* imgPtrOut;
//  for(int y=0; y<imgOut.rows; ++y){
//    imgPtrInTop =  imgIn.data + 2*y*refStepIn;
//    imgPtrInBot =  imgIn.data + (2*y+1)*refStepIn;
//    imgPtrOut = imgOut.data + y*refStepOut;
//    for(int x=0; x<imgOut.cols; ++x, ++imgPtrOut, imgPtrInTop += 2, imgPtrInBot += 2)
//    {
//      *imgPtrOut = (imgPtrInTop[0]+imgPtrInTop[1]+imgPtrInBot[0]+imgPtrInBot[1])/4;
//    }
//  }
//}

/** \brief Image pyramid with selectable number of levels.
 *
 *   @tparam n_levels - Number of pyramid levels.
 *   @todo: complete rule of three
 */
template<int n_levels>
class ImagePyramid{
 public:
  ImagePyramid(){};
  virtual ~ImagePyramid(){};
  cv::Mat imgs_[n_levels]; /**<Array, containing the pyramid images.*/
  cv::Point2f centers_[n_levels]; /**<Array, containing the image center coordinates (in pixel), defined in an
                                      image centered coordinate system of the image at level 0.*/

   


  /** \brief Initializes the image pyramid from an input image (level 0).
   *
   *   @param img   - Input image (level 0).
   *   @param useCv - Set to true, if opencv (cv::pyrDown) should be used for the pyramid creation.
   */
  void computeFromImage(const cv::Mat& img, const bool useCv = false){
    img.copyTo(imgs_[0]);
    centers_[0] = cv::Point2f(0,0);
    for(int i=1; i<n_levels; ++i){
      cv::pyrDown(imgs_[i-1],imgs_[i],cv::Size(imgs_[i-1].cols/2, imgs_[i-1].rows/2));
      centers_[i].x = centers_[i-1].x-pow(0.5,2-i)*(float)((imgs_[i-1].rows%2)+1);
      centers_[i].y = centers_[i-1].y-pow(0.5,2-i)*(float)((imgs_[i-1].cols%2)+1);
    }
  }

  /** \brief Copies the image pyramid.
   * @Note This only does a shallow copy of the images.
   */
  ImagePyramid<n_levels>& operator=(const ImagePyramid<n_levels> &rhs) {
    for(unsigned int i=0;i<n_levels;i++){
      rhs.imgs_[i].copyTo(imgs_[i]);
      centers_[i] = rhs.centers_[i];
    }
    return *this;
  }

  /** \brief Transforms pixel coordinates between two pyramid levels.
   *
   * @Note Invalidates camera and bearing vector, since the camera model is not valid for arbitrary image levels.
   * @param cIn        - Input coordinates
   * @param cOut       - Output coordinates
   * @param l1         - Input pyramid level.
   * @param l2         - Output pyramid level.
   * @return the corresponding pixel coordinates on pyramid level l2.
   */
  FeatureCoordinates levelTranformCoordinates(const FeatureCoordinates& cIn, const int l1, const int l2) const{
    assert(l1<n_levels && l2<n_levels && l1>=0 && l2>=0);

    FeatureCoordinates cOut;
    cOut.set_c((centers_[l1]-centers_[l2])*pow(0.5,l2)+cIn.get_c()*pow(0.5,l2-l1));
    if(cIn.mpCamera_ != nullptr){
      if(cIn.com_warp_c()){
        cOut.set_warp_c(cIn.get_warp_c());
      }
    }
    cOut.camID_ = -1;
    cOut.mpCamera_ = nullptr;
    return cOut;
  }

  /** \brief Extract FastCorner coordinates
   *
   * @param candidates         - List of the extracted corner coordinates (defined on pyramid level 0).
   * @param l                  - Pyramid level at which the corners should be extracted.
   * @param detectionThreshold - Detection threshold of the used cv::FastFeatureDetector.
   *                             See http://docs.opencv.org/trunk/df/d74/classcv_1_1FastFeatureDetector.html
   * @param valid_radius       - Radius inside which a feature is considered valid (as ratio of shortest image side)
   */
//  void detectFastCorners(FeatureCoordinatesVec & candidates, int l, int detectionThreshold, double valid_radius = std::numeric_limits<double>::max()) const{ // Origninal
    void detectFastCorners(FeatureCoordinatesVec &candidates, int l, int detectionThreshold, const int camID,double valid_radius = std::numeric_limits<double>::max(), bool applyHistogramEqualization = false) const{ // Custom from rotio
    std::vector<cv::KeyPoint> keypoints;

    //CUSTOMIZATION from rotio
    //If image is 16-bit it is histrogram equalized and then converted to 8-bit for FAST, otherwise if 8-bit image range convert from float to 8-bit
    cv::Mat histEqualizedLevelImg;
    if (applyHistogramEqualization)
        equalizeHistogram(l, histEqualizedLevelImg);
    else
        imgs_[l].convertTo(histEqualizedLevelImg, CV_8UC1); //smk: needed as original input image is saved as float now, which is not accepted by FAST detector
    //CUSTOMIZATION

    //RYB STUFF:
    //RYB: mask used to limit the searchable area for feature descriptors in the image. 
    cv::Mat circle_mask = cv::Mat::zeros(histEqualizedLevelImg.size(), CV_8U);
    int img_cols = histEqualizedLevelImg.cols;
    int img_rows = histEqualizedLevelImg.rows;
    cv::circle(circle_mask, cv::Point(img_cols/2,img_rows/2),(img_rows/4)-3, cv::Scalar(255),CV_FILLED, 8,0);

#if (CV_MAJOR_VERSION < 3)
    cv::FastFeatureDetector feature_detector_fast(detectionThreshold, true);
    feature_detector_fast.detect(histEqualizedLevelImg, keypoints, circle_mask); //CUSTOMIZATION
#else
    auto feature_detector_fast = cv::FastFeatureDetector::create(detectionThreshold, true);
    feature_detector_fast->detect(histEqualizedLevelImg, keypoints, circle_mask); //CUSTOMIZATION
#endif

    candidates.reserve(candidates.size()+keypoints.size());
    for (auto it = keypoints.cbegin(), end = keypoints.cend(); it != end; ++it) {

      const double x_dist = it->pt.x - imgs_[l].cols/2.0;
      const double y_dist = it->pt.y - imgs_[l].rows/2.0;
      const double max_valid_dist = valid_radius*std::min(imgs_[l].cols, imgs_[l].rows);

      if((x_dist*x_dist + y_dist*y_dist) > (max_valid_dist*max_valid_dist)){
        continue;
      }

      candidates.push_back(
              levelTranformCoordinates(FeatureCoordinates(cv::Point2f(it->pt.x, it->pt.y)),l,0));
      candidates.back().camID_ = camID;
    }
  }

    //Description: This function takes in an image and improves its contrast by equalizing its histogram.
    //This function is used for equalization of 16-bit thermal images which cannot be improved cv::equalizeHist function.
    //First a histogram of the image is calculated.
    //Then useful range is computed by clipping intensity containing at least N pixels in bin from beginning and end of the histogram.
    //The useful range is then mapped to 8-bit image range of 0-255(outImg).
    void equalizeHistogram(int lvl, cv::Mat &outImg) const
    {
        //Set Min and Max range for 16-bit
        float intensityMin = 0.0, intensityMax = 65535.0;
        int histSize = intensityMax - intensityMin + 1;
        float range[] = {intensityMin, intensityMax};
        const float *histRange = {range};
        cv::Mat inImg;
        imgs_[lvl].copyTo(inImg); // Custom rovtio Deep copy after normal copy constructor is made shallow.

        //Create histogram
        cv::Mat histogram;
        //RYB:-------
        //cv::Mat circle_mask = cv::Mat::zeros(inImg.size(), CV_8U);
        //int img_cols = inImg.cols;
        //int img_rows = inImg.rows;
        //cv::circle(circle_mask, cv::Point(img_cols/2,img_rows/2),(img_rows/4), cv::Scalar(255),CV_FILLED, 8,0);
        //cv::calcHist(&inImg, 1, 0, circle_mask, histogram, 1, &histSize, &histRange, true, false);
        //--------
        cv::calcHist(&inImg, 1, 0, cv::noArray(), histogram, 1, &histSize, &histRange, true, false);

        //Find clipping range w.r.t minimmum and maximum intensity values (CDF) with atleast N=10000 pixels in bin
        int cummulativePixels = 10000 * std::pow(0.5, lvl);
        //Min value
        int bin = 1; //ignoring 0 bin
        for (int sum = 0; sum < cummulativePixels; sum += histogram.at<float>(bin), ++bin)
            ;
        double min_val = bin - 1;
        //Max value
        bin = histSize - 1;
        for (int sum = 0; sum < cummulativePixels; sum += histogram.at<float>(bin), --bin);
        double max_val = bin + 1;

        //Scale and convert to 8-bit
        //reference: http://answers.opencv.org/question/75510/how-to-make-auto-adjustmentsbrightness-and-contrast-for-image-android-opencv-image-correction/?answer=75797#post-id-75797
        double alpha = 255.0 / (max_val - min_val);
        double beta = -1.0 * min_val * alpha;
        inImg.convertTo(outImg, CV_8UC1, alpha, beta);
    }
};

}


#endif /* IMAGEPYRAMID_HPP_ */
