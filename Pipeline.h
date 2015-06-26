/*
 * Pipeline.h
 *
 *  Created on: Jun 1, 2015
 *      Author: Simon Fojtu
 */

#ifndef PIPELINE_H_
#define PIPELINE_H_

#include <cv.h>
#include <cvaux.h>
#include <highgui.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "jansson.h"
#include "input.h"

#ifdef _MSC_VER
#include "winjunk.hpp"
#else
#ifndef CLASS_DECLSPEC
#define CLASS_DECLSPEC
#endif
#endif

using namespace cv;
using namespace std;

namespace firesight {

	CLASS_DECLSPEC typedef map<string,const char *> ArgMap;
	CLASS_DECLSPEC extern ArgMap emptyMap;


  typedef struct MatchedRegion {
    Range xRange;
    Range yRange;
    Point2f  average;
    int pointCount;
    float ellipse; // area of ellipse that covers {xRange, yRange}
    float covar;

    MatchedRegion(Range xRange, Range yRange, Point2f average, int pointCount, float covar);
    string asJson();
    json_t *as_json_t();
  } MatchedRegion;


  typedef class HoleRecognizer {
#define HOLE_SHOW_NONE 0 /* do not show matches */
#define HOLE_SHOW_MSER 1 /* show all MSER matches */
#define HOLE_SHOW_MATCHES 2 /* only show MSER matches that meet hole criteria */
    public:
      HoleRecognizer(float minDiameter, float maxDiameter);

//      bool apply(json_t *pStage, json_t *pStageModel, Model &model) {
//    	  return apply_absdiff(pStage, pStageModel, model);
//      }

      /**
       * Update the working image to show MSER matches.
       * Image must have at least three channels representing RGB values.
       * @param show matched regions. Default is HOLE_SHOW_NONE
       */
      void showMatches(int show);

      void scan(Mat &matRGB, vector<MatchedRegion> &matches, float maxEllipse = 1.05, float maxCovar = 2.0);

    private:
      int _showMatches;
      MSER mser;
      float minDiam;
      float maxDiam;
      int delta;
      int minArea;
      int maxArea;
      float maxVariation;
      float minDiversity;
      int max_evolution;
      float area_threshold;
      float min_margin;
      int edge_blur_size;
      void scanRegion(vector<Point> &pts, int i,
        Mat &matRGB, vector<MatchedRegion> &matches, float maxEllipse, float maxCovar);

  } MSER_holes;

    typedef struct Circle {
        float x;
        float y;
        float radius;

        Circle(float x, float y, float radius);
        string asJson();
        json_t *as_json_t();
    } Circle;

  typedef class HoughCircle {
#define CIRCLE_SHOW_NONE 0 /* do not show circles */
#define CIRCLE_SHOW_ALL 1  /* show all circles */
    public:
      HoughCircle(int minDiameter, int maxDiameter);

      /**
       * Update the working image to show detected circles.
       * Image must have at least three channels representing RGB values.
       * @param show matched regions. Default is CIRCLE_SHOW_NONE
       */
      void setShowCircles(int show);

      void scan(Mat &matRGB, vector<Circle> &circles);

      void setFilterParams( int d, double sigmaColor, double sigmaSpace);
      void setHoughParams(double dp, double minDist, double param1, double param2);

    private:
      int _showCircles;
      int minDiam;
      int maxDiam;
      vector<Circle> circles;

      void show(Mat & image, vector<Circle> circles);

      // bilateral filter parameters
      int bf_d;
      double bf_sigmaColor;
      double bf_sigmaSpace;

      // HoughCircle parameters
      double hc_dp;
      double hc_minDist;
      double hc_param1;
      double hc_param2;

  } HoughCircle;

  typedef struct XY {
      double x, y;
      XY(): x(0), y(0) {}
      XY(double x_, double y_): x(x_), y(y_) {}
  } XY;

  typedef class Pt2Res {
      public:

          Pt2Res() {}
          double getResolution(double thr1, double thr2, double confidence, double separation, vector<XY> coords);
      private:
          static bool compare_XY_by_x(XY a, XY b);
          static bool compare_XY_by_y(XY a, XY b);
          int nsamples_RANSAC(size_t ninl, size_t xlen, unsigned int NSAMPL, double confidence);
          static double _RANSAC_line(XY * x, size_t nx, XY C);
          static double _RANSAC_pattern(XY * x, size_t nx, XY C);
          vector<XY> RANSAC_2D(unsigned int NSAMPL, vector<XY> coords, double thr, double confidence, double(*err_fun)(XY *, size_t, XY));
          void least_squares(vector<XY> xy, double * a, double * b);

  } Pt2Res;

#ifdef LGPL2_1
  typedef struct QRPayload {
      double x, y;
      string text;
      json_t * as_json_t() {
          json_t *pObj = json_object();
          json_object_set(pObj, "x", json_real(x));
          json_object_set(pObj, "y", json_real(y));
          json_object_set(pObj, "text", json_string(text.c_str()));
          return pObj;
      }
      string asJson() {
          json_t *pObj = as_json_t();
          char *pObjStr = json_dumps(pObj, JSON_PRESERVE_ORDER|JSON_COMPACT|JSON_INDENT(2));
          string result(pObjStr);
          return result;
      }
  } QRPayload;

  typedef class ZbarQrDecode {
      public:
          ZbarQrDecode() {}
          vector<QRPayload> scan(Mat &img, int show);
  } ZbarQrDecode;
#endif // LGPL2_1

  typedef class StageData {
    public:
      StageData(string stageName);
      ~StageData();
  } StageData, *StageDataPtr;

  typedef class Model {
    private:
      json_t *pJson;

    public: // methods
      Model(ArgMap &argMap=emptyMap);
      ~Model();

      /**
       * Return JSON root node of image recognition model.
       * Caller must json_decref() returned value.
       */
      inline json_t *getJson(bool incRef = true) {
        if (incRef) {
          return json_incref(pJson);
        } else {
          return pJson;
        }
      };

    public: // fields
      Mat image;
      map<string, Mat> imageMap;
      map<string, StageDataPtr> stageDataMap;
      ArgMap argMap;
  } Model;

    class Stage;

    class Parameter {
    public:
        Parameter(Stage * stage) :
            stage(stage)
        {}
        virtual string toString() const { return "dummy"; }
        virtual void inc() = 0;
        virtual void dec() = 0;
    private:
        Stage * stage;
    };

    class IntParameter : public Parameter {
    public:
        IntParameter(Stage * stage, int& value) :
            Parameter(stage), value(value)
        {}
        string toString() const { return std::to_string(value); }
        void inc() { value++; }
        void dec() { value--; }
    private:
        int& value;
    };

    class BoolParameter : public Parameter {
    public:
        BoolParameter(Stage * stage, bool& value) :
            Parameter(stage), value(value)
        {}
        string toString() const { return (value ? "true" : "false"); }
        void inc() { value = !value; }
        void dec() { inc(); }
    private:
        bool& value;
    };

    class DoubleParameter : public Parameter {
    public:
        DoubleParameter(Stage * stage, double& value) :
            Parameter(stage), value(value)
        {}
        string toString() const { return std::to_string(value); }
        void inc() { value++; }
        void dec() { value--; }
    private:
        double& value;
    };

    class FloatParameter : public Parameter {
    public:
        FloatParameter(Stage * stage, float& value) :
            Parameter(stage), value(value)
        {}
        string toString() const { return std::to_string(value); }
        void inc() { value++; }
        void dec() { value--; }
    private:
        float& value;
    };

    class ScalarParameter : public Parameter {
    public:
        ScalarParameter(Stage * stage, Scalar& value) :
            Parameter(stage), value(value)
        {}
//        string toString() const { return "" }
        void inc() { ; }
        void dec() { ; }
    private:
        Scalar& value;
    };

    class StringParameter : public Parameter {
    public:
        StringParameter(Stage * stage, string& value) :
            Parameter(stage), value(value)
        {}
        string toString() const { return value; }
        void inc() { }
        void dec() { }
    private:
        string& value;
    };

    class SizeParameter : public Parameter {
    public:
        SizeParameter(Stage * stage, Size& value) :
            Parameter(stage), value(value)
        {}
        string toString() const { return std::to_string(value.width) + "x" + std::to_string(value.height); }
        void inc() { value.width++; value.height++; }
        void dec() { value.width--; value.height--; }
    private:
        Size& value;
    };

    class PointParameter : public Parameter {
    public:
        PointParameter(Stage * stage, Point& value) :
            Parameter(stage), value(value)
        {}
        string toString() const { return std::to_string(value.x) + ":" + std::to_string(value.y); }
        void inc() { value.x++; value.y++; }
        void dec() { value.x--; value.y--; }
    private:
        Point& value;
    };

    class EnumParameter : public Parameter {
    public:
        EnumParameter(Stage * stage, int& v, map<int, string> m) :
            Parameter(stage), value(v), _map(m)
        {}
        string toString() const { return _map.at(value); }
        void inc() {
            do {
                const int m = _map.rbegin()->first;
                value = (value + 1) % (m + 1);
            } while (_map.find(value) == _map.end());
        }
        void dec() {
            do {
                const int m = _map.rbegin()->first;
                value = (value + m) % (m + 1);
            } while (_map.find(value) == _map.end());
        }
    private:
        int& value;
        map<int, string> _map;
    };


    class Stage {
    public:
        Stage(json_t *pStage) : pStage(pStage), errMsg("") {}

        bool apply(json_t *pStageModel, Model &model) {
            bool result;

            //timer.start();

            result = apply_internal(pStageModel, model);

            //time = timer.nsecsElapsed();

            // remember error message
            const char * msg = json_string_value(json_object_get(pStageModel, "error"));
            if (msg)
                errMsg = String(msg);
            else
                errMsg = "";

            return result;
        }

        virtual string getName() const = 0;

        virtual string getErrorMessage() const { return errMsg; }

        virtual void print() const {
            printf("-- %s --\n", getName().c_str());
            for (auto it : _params)
                printf("%16s = %s\n", it.first.c_str(), it.second->toString().c_str());
        }

        virtual vector<string> info() const {
            vector<string> out;
            out.push_back(getName());
            for (auto it : _params)
                out.push_back(it.first + " = " + it.second->toString());
            return out;
        }

        virtual bool apply_internal(json_t *pStageModel, Model &model) = 0;

        static bool stageOK(const char *fmt, const char *errMsg, json_t *pStage, json_t *pStageModel);

        map<string, Parameter*>& getParams() { return _params; }
        void setParameter(string name, Parameter * value);

    protected:
        map<string, Parameter*> _params;
        json_t *pStage;
        string errMsg;
    };

    class StageFactory {
    public:
        static unique_ptr<Stage> getStage(const char *pOp, json_t *pStage, Model &model);
    };

  typedef class CLASS_DECLSPEC Pipeline {
    public:
      static void validateImage(Mat &image);
      bool stageOK(const char *fmt, const char *errMsg, json_t *pStage, json_t *pStageModel);
    protected:
      bool processModel(Model &model);
      bool processModelGUI(Input *input, Model &model);
      unique_ptr<Stage> parseStage(int index, json_t * pStage, Model &model);
//      bool processStage(int index, json_t *pStage, Model &model);
      KeyPoint _regionKeypoint(const vector<Point> &region);
      void _eigenXY(const vector<Point> &pts, Mat &eigenvectorsOut, Mat &meanOut, Mat &covOut);
      void _covarianceXY(const vector<Point> &pts, Mat &covOut, Mat &meanOut);
      bool morph(json_t *pStage, json_t *pStageModel, Model &model, String mop, const char * fmt) ;

      bool apply_absdiff(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_backgroundSubtractor(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_matchTemplate(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_calcHist(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_calcOffset(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_circle(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_convertTo(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_cout(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_detectParts(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_dft(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_dftSpectrum(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_dilate(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_drawKeypoints(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_drawRects(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_equalizeHist(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_erode(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_FireSight(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_HoleRecognizer(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_HoughCircles(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_points2resolution_RANSAC(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_sharpness(json_t *pStage, json_t *pStageModel, Model &model);
#ifdef LGPL2_1
      bool apply_qrdecode(json_t *pStage, json_t *pStageModel, Model &model);
#endif // LGPL2_1
      bool apply_log(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_Mat(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_matchGrid(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_meanStdDev(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_minAreaRect(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_model(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_morph(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_MSER(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_normalize(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_proto(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_PSNR(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_putText(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_rectangle(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_resize(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_threshold(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_warpRing(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_SimpleBlobDetector(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_split(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_stageImage(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_transparent(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_undistort(const char *pName, json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_warpAffine(json_t *pStage, json_t *pStageModel, Model &model);
      bool apply_warpPerspective(const char *pName, json_t *pStage, json_t *pStageModel, Model &model);

      void detectKeypoints(json_t *pStageModel, vector<vector<Point> > &regions);
      void detectRects(json_t *pStageModel, vector<vector<Point> > &regions);
      int parseCvType(const char *typeName, const char *&errMsg);
      json_t *pPipeline;

    public:
      enum DefinitionType { PATH, JSON };

      /**
       * Construct an image processing pipeline described by the given JSON array
       * that specifies a sequence of named processing stages.
       * @param pDefinition null terminated JSON string or file path
       * @param indicates whether definition is JSON string or file path
       */
      Pipeline(const char * pDefinition=NULL, DefinitionType defType=JSON );

      /**
       * Construct an image processing pipeline described by the given JSON array
       * that specifies a sequence of named processing stages.
       * @param pJson jansson array node
       */
      Pipeline(json_t *pJson);

      ~Pipeline();

      /**
       * Process the given working image and return a JSON object that represents
       * the recognized model comprised of the individual stage models.
       * @param input initial and transformed working image
       * @return pointer to jansson root node of JSON object that has a field for each recognized stage model. E.g., {s1:{...}, s2:{...}, ... , sN:{...}}
       */
      json_t *process(Input * input, ArgMap &argMap, Mat &output, bool gui = false);

  } Pipeline;

}

#endif /* PIPELINE_H_ */
