#ifndef _PLAYER_HH_
#define _PLAYER_HH_

#define FLAG_UP    0x01
#define FLAG_DOWN  0x02
#define FLAG_LEFT  0x04
#define FLAG_RIGHT 0x08

#define PSPEED 0.01  // player default speed
#define PUSIZE 1.0


namespace TEA {

	class Player {
		int _flags;
		double _xpos;
		double _ypos;

		public:
		~Player();
		Player(double x = 0, double y = 0);

		void tick(double);

		// accessors
		void set_flags(int);
		double get_xpos() const;
		double get_ypos() const;
		void set_pos(double, double);
	};
}

#endif
