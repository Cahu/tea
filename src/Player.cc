#include "Player.hh"


namespace TEA {

	Player::~Player()
	{
#ifndef NDEBUG
		fprintf(stderr, "- Player destroyed\n");
#endif
	}


	Player::Player(double x = 0, double y = 0)
	{
		_xpos = x;
		_ypos = y;
#ifndef NDEBUG
		fprintf(stderr, "+ Player crated\n");
#endif
	}


	// if time_ms < 0, you can rewind time!
	void Player::tick(double time_ms)
	{
		double xinc = 0;
		double yinc = 0;

		if (_flags & FLAG_UP)    { yinc = -1 * PSPEED * time_ms; }
		if (_flags & FLAG_DOWN)  { yinc = +1 * PSPEED * time_ms; }
		if (_flags & FLAG_LEFT)  { xinc = -1 * PSPEED * time_ms; }
		if (_flags & FLAG_RIGHT) { xinc = +1 * PSPEED * time_ms; }

		// moving diagonaly? Normalize with 1/sqrt(2)
		if (xinc && yinc) {
			xinc *= 0.7071;
			yinc *= 0.7071;
		}

		_xpos += xinc;
		_ypos += yinc;
	}


	void Player::set_flags(int flags)
	{
		_flags = flags;
	}


	double Player::get_xpos()
	{
		return _xpos;
	}


	double Player::get_ypos()
	{
		return _ypos;
	}
}
