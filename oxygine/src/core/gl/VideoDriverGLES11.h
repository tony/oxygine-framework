#pragma once
#include "oxygine_include.h"
#include "VideoDriverGL.h"

namespace oxygine
{
	class VideoDriverGLES11: public VideoDriverGL
	{
	public:
		VideoDriverGLES11();
		~VideoDriverGLES11();

		spNativeTexture createTexture();

		bool isReady() const {return true;}
		void reset(){}
		void restore(){}

		void begin(const Matrix &proj, const Matrix &view, const Rect &viewport, const Color *clearColor);
		
		void setDefaultSettings();
	};
}