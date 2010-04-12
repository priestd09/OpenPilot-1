
#include "correl/zncc.hpp"

#include <stdint.h>

namespace jafar {
namespace correl {

	// a bit faster than normal implementation, but a bit less precise (but it should be ok even for a full image)
	// all ifs on boolean template parameters cost nothing
	template<int depth, typename worktype, typename bornetype, bornetype borneinf, bornetype bornesup, bool useBornes, bool useWeightMatrix>
	double Zncc::computeTpl(image::Image const* im1, image::Image const* im2, float const* weightMatrix)
	{
		// preconds
		JFR_PRECOND( im1->depth() == depth, "Image 1 depth is different from the template parameter" );
		JFR_PRECOND( im2->depth() == depth, "Image 2 depth is different from the template parameter" );
		JFR_PRECOND( im1->channels() == im2->channels(), "The channels number of both images are different" );
		JFR_PRECOND( !useWeightMatrix || weightMatrix, "Template parameter tells to use weightMatrix but no one is given" );
		
		CvRect roi1 = im1->roi();
		CvRect roi2 = im2->roi();
		JFR_PRECOND( roi1.width  == roi2.width , "The width of both images or roi are different" );
		JFR_PRECOND( roi1.height == roi2.height, "The height of both images or roi are different" );
		
		// reduce rois if a part is outside of the image
		int delta;
		if (roi1.x < 0) { roi2.x -= roi1.x; roi2.width += roi1.x; roi1.width = roi2.width; roi1.x = 0; }
		if (roi2.x < 0) { roi1.x -= roi2.x; roi1.width += roi2.x; roi2.width = roi1.width; roi2.x = 0; }
		if (roi1.y < 0) { roi2.y -= roi1.y; roi2.height += roi1.y; roi1.height = roi2.height; roi1.y = 0; }
		if (roi2.y < 0) { roi1.y -= roi2.y; roi1.height += roi2.y; roi2.height = roi1.height; roi2.y = 0; }
		delta = im1->width()-(roi1.x+roi1.width); if (delta < 0) { roi1.width += delta; roi2.width = roi1.width; }
		delta = im2->width()-(roi2.x+roi2.width); if (delta < 0) { roi2.width += delta; roi1.width = roi2.width; }
		delta = im1->height()-(roi1.y+roi1.height); if (delta < 0) { roi1.height += delta; roi2.height = roi1.height; }
		delta = im2->height()-(roi2.y+roi2.height); if (delta < 0) { roi2.height += delta; roi1.height = roi2.height; }

		// some variables initialization
		unsigned roi1_step, roi2_step;
		roi1_step = im1->step() - roi1.width;
		roi2_step = im2->step() - roi2.width;
		
		double mean1 = 0., mean2 = 0.;
		double sigma1 = 0., sigma2 = 0.;
		double zncc_sum = 0.;
		double zncc_count = 0.;
		double zncc_total = 0.;
		
		worktype const* im1ptr = reinterpret_cast<worktype const*>(im1->data());
		worktype const* im2ptr = reinterpret_cast<worktype const*>(im2->data());
		im1ptr += roi1.y*im1->step()+roi1.x;
		im2ptr += roi2.y*im2->step()+roi2.x;
		float const* wptr = weightMatrix;
		double w;
		
		// start the loops
		for(int i = 0; i < roi1.height; ++i) 
		{
			for(int j = 0; j < roi1.width; ++j) 
			{
				worktype im1v = *(im1ptr++);
				worktype im2v = *(im2ptr++);
				if (useWeightMatrix) w = *(wptr++); else w = 1;
				if (useBornes) zncc_total += w;
				
				if (!useBornes || (im1v != borneinf && im1v != bornesup && im2v != borneinf && im2v != bornesup))
				{
#if 0
					double im1vw, im2vw;
					if (useWeightMatrix)
						{ im1vw = im1v * w; im2vw = im2v * w; } else
						{ im1vw = im1v;     im2vw = im2v;     }
					zncc_count += w;
					mean1 += im1vw;
					mean2 += im2vw;
					sigma1 += im1v * im1vw;
					sigma2 += im2v * im2vw;
					zncc_sum += im1v * im2vw;
#else
					zncc_count += w;
					mean1 += im1v * w;
					mean2 += im2v * w;
					sigma1 += im1v * im1v * w;
					sigma2 += im2v * im2v * w;
					zncc_sum += im1v * im2v * w;
#endif
				}
			}
			im1ptr += roi1_step;
			im2ptr += roi2_step;
		}
		
		if (useBornes) if (zncc_count / zncc_total < 0.5) return 0;
		
		// finish
		mean1 /= zncc_count;
		mean2 /= zncc_count;
		sigma1 = sqrt(sigma1/zncc_count - mean1*mean1);
		sigma2 = sqrt(sigma2/zncc_count - mean2*mean2);
		zncc_sum = (zncc_sum/zncc_count - mean1*mean2) / (sigma1*sigma2);
		
		return zncc_sum;
	}


	double Zncc::compute(image::Image const* im1, image::Image const* im2, float const* weightMatrix)
	{
		JFR_PRECOND(im1->depth() == im2->depth(), "The depth of both images is different");
		switch(im1->depth())
		{
			case IPL_DEPTH_1U:
				if (weightMatrix == NULL)
					return computeTpl<IPL_DEPTH_1U, bool,bool,0,1,true,false>(im1,im2);
				else
					return computeTpl<IPL_DEPTH_1U, bool,bool,0,1,true,true>(im1,im2,weightMatrix);
			case IPL_DEPTH_8U:
				if (weightMatrix == NULL)
					return computeTpl<IPL_DEPTH_8U, uint8_t,uint8_t,0,255,true,false>(im1,im2);
				else
					return computeTpl<IPL_DEPTH_8U, uint8_t,uint8_t,0,255,true,true>(im1,im2,weightMatrix);
			case IPL_DEPTH_8S:
				if (weightMatrix == NULL)
					return computeTpl<IPL_DEPTH_8S, int8_t,int8_t, -128,127,true,false>(im1,im2);
				else
					return computeTpl<IPL_DEPTH_8S, int8_t,int8_t, -128,127,true,true>(im1,im2,weightMatrix);
			case IPL_DEPTH_16U:
				if (weightMatrix == NULL)
					return computeTpl<IPL_DEPTH_16U, uint16_t,uint16_t, 0,65535,true,false>(im1,im2);
				else
					return computeTpl<IPL_DEPTH_16U, uint16_t,uint16_t, 0,65535,true,true>(im1,im2,weightMatrix);
			case IPL_DEPTH_16S:
				if (weightMatrix == NULL)
					return computeTpl<IPL_DEPTH_16S, int16_t,int16_t, -32768,32767,true,false>(im1,im2);
				else
					return computeTpl<IPL_DEPTH_16S, int16_t,int16_t, -32768,32767,true,true>(im1,im2,weightMatrix);
			case IPL_DEPTH_32F:
				if (weightMatrix == NULL) // bool and no borne because cannot use a float as a template parameter, and anyway would be useless here
					return computeTpl<IPL_DEPTH_32F, float,bool, 0,0,false,false>(im1,im2);
				else
					return computeTpl<IPL_DEPTH_32F, float,bool, 0,0,false,true>(im1,im2,weightMatrix);
			default:
				JFR_PRECOND(false, "Unknown image depth");
				return FP_NAN;
		}
	}

	#if 1
	// TODO : test / maybe improve it to manage more nicely rois
	double Zncc::exploreRotation(image::Image const* im1, image::Image const* im2, int rotationStep)
	{
		CvRect roi2 = im2->roi();
//		int dim = (roi2->width > roi2->height ? roi2->width : roi2->height)*1.5;// 1.5 > sqrt(2)
//		image::Image* im2bis = new image::Image(dim, dim, im2->depth(), im2->colorSpace());
		image::Image* im2bis = new image::Image(roi2.width, roi2.height, im2->depth(), im2->colorSpace());
		double bestZncc = compute(im1,im2);
	//   double tempBestAngle = 0.;
		for(int angle = rotationStep; angle < 360; angle += rotationStep)
		{
			float m[6];
			int w = im2bis->width();
			int h = im2bis->height();
			float radangle = angle * CV_PI/180.;
			m[0] = (float)(cos(radangle));
			m[1] = (float)(sin(radangle));
			m[2] = w * 0.5f;
			m[3] = -m[1];
			m[4] = m[0];
			m[5] = h * 0.5f;
			CvMat M = cvMat( 2, 3, CV_32F, m );
			cvFillImage(*im2bis, 0);
			cvGetQuadrangleSubPix( *im2, *im2bis, &M);
			double zncc = compute(im1,im2bis);
			if(zncc > bestZncc) { bestZncc = zncc; /*tempBestAngle = radangle;*/}
		}
	//   JFR_DEBUG(tempBestAngle);
		delete im2bis;
		return bestZncc;
	}
	#endif


	#if 0
	// the normal algorithm in two steps, for reference, maybe we should provide it

	{
			// Compute the mean
			worktype const* im1ptr = reinterpret_cast<worktype const*>(im1->data());
			worktype const* im2ptr = reinterpret_cast<worktype const*>(im2->data());
			im1ptr += roi1.y*im1->step()+roi1.x;
			im2ptr += roi2.y*im2->step()+roi2.x;
			
			double mean1 = 0.;
			double mean2 = 0.;
			int zncc_count = 0;
			
			for(int i = 0; i < roi1.height; ++i) 
			{
				for(int j = 0; j < roi1.width; ++j) 
				{
					worktype im1v = *(im1ptr++);
					worktype im2v = *(im2ptr++);
					if(im1v != borneinf && im1v != bornesup && im2v != borneinf && im2v != bornesup )
					{
						mean1 += im1v;
						mean2 += im2v;
						zncc_count++;
					}
				}
				im1ptr += roi1_step;
				im2ptr += roi2_step;
			}
			mean1 /= zncc_count;
			mean2 /= zncc_count; 
		
			// Compute the sigma and the zncc sum
			im1ptr = reinterpret_cast<worktype const*>(im1->data());
			im2ptr = reinterpret_cast<worktype const*>(im2->data());
			im1ptr += roi1.y*im1->step()+roi1.x;
			im2ptr += roi2.y*im2->step()+roi2.x;
			
			double sigma1 = 0.;
			double sigma2 = 0.;
			double zncc_sum = 0.;
			
			for(int i = 0; i < roi1.height; ++i) 
			{
				for(int j = 0; j < roi1.width; ++j) 
				{
					worktype im1v = *(im1ptr++);
					worktype im2v = *(im2ptr++);
					if(im1v != borneinf && im1v != bornesup && im2v != borneinf && im2v != bornesup )
					{
						sigma1 += details::pow2( im1v - mean1 );
						sigma2 += details::pow2( im2v - mean2 );
						zncc_sum += im1v * im2v;
					}
				}
				im1ptr += roi1_step;
				im2ptr += roi2_step;
			}
			sigma1 = sqrt(sigma1 / zncc_count);
			sigma2 = sqrt(sigma2 / zncc_count);
			zncc_sum = zncc_sum / zncc_count - mean1 * mean2;
			
			return zncc_sum / (sigma1 * sigma2);
	}
	#endif



}
}
