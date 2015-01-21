#pragma once
#include "oxygine_include.h"
#include "core/Texture.h"
#include "math/Rect.h"
#include "res/ResAnim.h"
#include "Actor.h"
#include "AnimationFrame.h"
#include "VisualStyle.h"

namespace oxygine
{
	class ResAnim;			

	DECLARE_SMART(Sprite, spSprite);
    class Sprite : public _VStyleActor
	{
	public:
		DECLARE_COPYCLONE_NEW(Sprite);

		Sprite();
		~Sprite();
		
		const AnimationFrame&	getAnimFrame() const {return _frame;}
		virtual RectF			getDestRect() const;
		const Diffuse&			getDiffuse() const {return _frame.getDiffuse();}
		bool					getManageResAnim() const {return (_flags & flag_manageResAnim) != 0;}
		const RectF&			getSrcRect() const {return _frame.getSrcRect();}
		const ResAnim*			getResAnim() const{return _frame.getResAnim();}
		int						getColumn() const {return _frame.getColumn();}
		int						getRow() const {return _frame.getRow();}

		/**load/unload atlas automatically or not*/
		void					setManageResAnim(bool manage);
		/**Changes sprite image*/
		void					setAnimFrame(const AnimationFrame &f);
		/**Takes AnimationFrame from ResAnim and set it as current to Sprite. Shows assert is resanim is null. Using this method is more safe than 'setAnimFrame(const AnimationFrame &f)'*/
		void					setAnimFrame(const ResAnim *resanim, int col = 0, int row = 0);
		virtual void			setResAnim(const ResAnim *resanim);
		void					setRow(int row, int column = -1);
		void					setColumn(int column, int row = -1);
		
		void serialize(serializedata* data);
		void deserialize(const deserializedata* data);
		
		std::string dump(const dumpOptions &) const;
	protected:
		enum 
		{
			flag_manageResAnim = flag_last << 1
		};
		virtual void changeAnimFrame(const AnimationFrame &f);
		virtual void animFrameChanged(const AnimationFrame &f);
		void doRender(const RenderState &rs);

		AnimationFrame _frame;
	};


	/** A TweenAnim class
	*   use for playing per frame animation
	\code
	spSprite sprite = new Sprite();
	sprite->addTween(TweenAnim(res.getResAnim("anim")), 500, -1);
	\endcode
	*/
	class TweenAnim
	{
	public:
		typedef Sprite type;

		TweenAnim(const ResAnim *resAnim, int row = 0);

		void init(Sprite &actor);

		const ResAnim*	getResAnim() const {return _resAnim;}
		int				getRow() const {return _row;}
		int				getStartColumn() const {return _colStart;}
		int				getEndColumn() const {return _colEnd;}

		
		void setResAnim(const ResAnim *resAnim);
		/**play animation in interval [start, end)*/
		void setColumns(int start, int end);
		
		void update(Sprite &actor, float p, const UpdateState &us);

	private:
		const ResAnim *_resAnim;
		int _row;
		int _colStart;
		int _colEnd;
	};
}

#ifdef OX_EDITOR
#include "EditorSprite.h"
#else
namespace oxygine
{
	typedef Sprite _Sprite;
}
#endif
