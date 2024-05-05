#include "Player.hh"

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Bernat


struct PLAYER_NAME : public Player {

  /**
   * Factory: returns a new instance of this class.
   * Do not modify this function.
   */
  static Player* factory () {
    return new PLAYER_NAME;
  }

  /**
   * Types and attributes for your player can be defined here.
   */
  #define num_players 4
  #define num_days 5
  #define num_rounds_per_day 50
  #define num_rounds 250
  #define board_rows 15
  #define board_cols 30
  #define num_ini_builders 4
  #define num_ini_warriors 2
  #define num_ini_money 10
  #define num_ini_food 5
  #define num_ini_guns 4
  #define num_ini_bazookas 2
  #define builder_ini_life 60
  #define warrior_ini_life 100
  //#define citizen_ini_life(CitizenType ct)
  #define money_points 5
  #define kill_builder_points 100
  #define kill_warrior_points 250
  #define food_incr_life 20
  #define life_lost_in_attack 20
  #define builder_strength_attack 1
  #define hammer_strength_attack 10
  #define gun_strength_attack 100
  #define bazooka_strength_attack 1000
  //#define weapon_strength_attack(WeaponType w)
  #define builder_strength_demolish 3
  #define hammer_strength_demolish 10
  #define gun_strength_demolish 10
  #define bazooka_strength_demolish 30
  //#define weapon_strength_demolish(WeaponType w)
  #define num_rounds_regen_builder 50
  #define num_rounds_regen_warrior 50
  //#define num_rounds_regen_citizen(CitizenType ci)
  #define num_rounds_regen_food 10
  #define num_rounds_regen_money 5
  #define num_rounds_regen_weapon 40
  #define barricade_resistance_step 40
  #define barricade_max_resistance 320
  #define max_num_barricades 3
  //#define player_ok (int pl)
  //#define pos_ok (int i, int j)
  //#define pos_ok (Pos p)
  //#define is_round_night (int r)
  //#define is_round_day (int r)

  vector<set<Pos>> weapons = vector<set<Pos>> (2);
  queue<int> next_weapon;
  vector<Dir> Directions = {Down,Right,Up,Left};

  struct Board_cell {
    int dist;
    vector<int> dirs;
    bool vist;

    Board_cell() {
      dist = -1;
      vist = false;
    }
  };

  struct Board {
    vector<vector<Board_cell>> B;

    Board() {
      B = vector<vector<Board_cell>> (board_rows, vector<Board_cell> (board_cols));
    }

    Board_cell& operator[](Pos p) {
      return B[p.i][p.j];
    }
  };
  
  int days_to_move(int id, int rd, Cell c) {
    int days_to_start = 0;
    int days_to_break = 0;
    if (c.b_owner != -1 and c.b_owner != me() and is_round_day(rd)) days_to_start = 25-rd%25;
    switch (citizen(id).weapon) {
      case Hammer: 
        days_to_break = c.resistance/hammer_strength_demolish;
        break;
      case Gun:
        days_to_break = c.resistance/gun_strength_demolish;
        break;
      case Bazooka:
        days_to_break = c.resistance/bazooka_strength_demolish;
        break;
      case NoWeapon:
        days_to_break = c.resistance/builder_strength_demolish;
        break;
    }
    return days_to_start + days_to_break + 1;
  }

  double bonus_value(Pos p, double dist, int id) {
    if (cell(p).bonus == Money) return 10./dist;
    double x = ceil((double) citizen(id).life/life_lost_in_attack);
    return 1000./(x*x*x*x*dist);
  }

  int tier(WeaponType w) {
    switch (w) {
      case NoWeapon: return 0;
      case Hammer: return 1;
      case Gun: return 2;
      case Bazooka: return 3;
      default: return -1;
    }
  }

  double weapon_value(Pos p, double dist, int id) {
    return 0.;
  }

  double citizen_value(Pos p, double dist, int id) {
    double value = 0;
    double myTier = tier(citizen(id).weapon);
    double hisTier = tier(citizen(cell(p).id).weapon);
    double myHelth = ceil((double) citizen(id).life/life_lost_in_attack);
    double hisHelth = ceil((double) citizen(cell(p).id).life/life_lost_in_attack);
    double hisPoints = score(citizen(cell(p).id).player);
    double days2night = num_rounds_per_day/2 - round()%(num_rounds_per_day/2);
    double days2day = num_rounds_per_day/2 - round()%(num_rounds_per_day/2);
    if (myTier - hisTier < 0 and is_day()) {
      value = -(1./myHelth)*((hisPoints/100.+1000.)/(days2night*(dist/2.)*(dist/2.)));
    }
    else if (myTier - hisTier < 0 and is_night()) {
      if (days2day < dist/2.) value = 0;
      else value = -(1./myHelth)*((hisPoints/100.+1000.)/((dist/2.)*(dist/2.)))*(days2day-dist/2.);
    }
    else if (myTier - hisTier == 0 and is_day()) {
      value = ((myHelth-hisHelth)/7.)*((hisPoints/100.+1000.)/(days2night*(dist/2.)*(dist/2.)));
    }
    else if (myTier - hisTier == 0 and is_night()) {
      if (days2day < dist/2.) value = 0;
      else value = ((myHelth-hisHelth)/7.)*((hisPoints/100.+1000.)/((dist/2.)*(dist/2.)))*(days2day-dist/2.);
    }
    else if (myTier - hisTier > 0 and is_day()) {
      value = (1.-1./(2.*myHelth*myHelth))*((100.*hisTier+hisPoints/100.+1000.)/(days2night*(dist/2.)*(dist/2.)));
    }
    else if (myTier - hisTier > 0 and is_night()) {
      if (days2day < dist/2.) value = 0;
      else value = (1.-1./(2.*myHelth*myHelth))*((100.*hisTier+hisPoints/100.+1000.)/((dist/2.)*(dist/2.)))*(days2day-dist/2.);
    }
    return value;
  }

  void find(int id) {
    Board B;
    priority_queue<pair<int,Pos>, vector<pair<int,Pos>>, greater<pair<int,Pos>>> q;
    set<Pos> bonus;
    set<Pos> weapons;
    set<Pos> citizens;

    Pos a = citizen(id).pos;
    B[a].dist = 0;
    B[a].vist = true;
    for (Dir d: Directions) {
      if (pos_ok(a+d) and cell(a+d).type == Street) {
        int dist = days_to_move(id, round(), cell(a+d));
        B[a+d].dist = dist;
        B[a+d].dirs = vector<int> (4,false);
        B[a+d].dirs[d] = true;
        q.push(make_pair(B[a+d].dist, a+d));
      }
    }

    while(not q.empty() and q.top().first < 10) {
      Pos b = q.top().second;
      q.pop();
      if (not B[b].vist) {
        B[b].vist = true;
        if (cell(b).bonus != NoBonus) bonus.insert(b);
        else if (cell(b).weapon != NoWeapon) weapons.insert(b);
        else if (cell(b).id != -1 and citizen(cell(b).id).player != me()) citizens.insert(b);
        for (Dir d : Directions) {
          if (pos_ok(b+d) and cell(b+d).type == Street) {
            int dist = days_to_move(id, round() + B[b].dist, cell(b+d));
            if (B[b+d].dist == -1 or B[b].dist + dist < B[b+d].dist) {
              B[b+d].dist = B[b].dist + dist;
              B[b+d].dirs = B[b].dirs;
              q.push(make_pair(B[b+d].dist,b+d));
            }
            else if (B[b].dist + dist == B[b+d].dist) {
              for (Dir dir : Directions) B[b+d].dirs[dir] = B[b+d].dirs[dir] or B[b].dirs[dir];
            }
          }
        }
      }
    }

    vector<double> v(4,0);
    for (Pos c : bonus) {
      for (Dir d : Directions) if (B[c].dirs[d]) v[d] += bonus_value(c, B[c].dist, id);
    }

    for (Pos c : weapons) {
      for (Dir d : Directions) if (B[c].dirs[d]) v[d] += weapon_value(c, B[c].dist, id);
    }

    for (Pos c : citizens) {
      for (Dir d : Directions) if (B[c].dirs[d]) v[d] += citizen_value(c, B[c].dist, id);
    }

    if(citizen(id).type == Builder and is_round_day(round()) and is_round_night(round()+1)) {
      Dir min = Dir(-1);
      for (Dir d : Directions) {
        if (min == -1 or v[d] > v[min]) min = d;
      }
      build(id, min);
    }

    Dir max = Dir(-1);
    for (Dir d : Directions) {
      if (max == -1 or v[d] > v[max]) max = d;
    }
    move(id, max);

//            ******DEBUG******
/*
    vector<char> aux = {'v','>','^','<'};
    cerr << a << endl;
    for (int i = 0; i < board_rows; ++i) {
      for (int ii = 0; ii < board_cols; ++ii) {
        cerr << '|';
        if (B[Pos(i,ii)].dist >= 0 and B[Pos(i,ii)].dist < 10) cerr << 0;
        cerr << B[Pos(i,ii)].dist;
        if (B[Pos(i,ii)].dist != -1 and B[Pos(i,ii)].dist != 0) {
          for (int d = 0; d < int(B[Pos(i,ii)].dirs.size()); ++d) {
            if(B[Pos(i,ii)].dirs[d]) cerr << aux[d];
            else cerr << '.';
          }
        }
        else cerr << "....";
        cerr << '|';
      }
      cerr << endl;
    }
    for (Dir d : Directions) {
      cerr << d << ':' << v[d] << ' ';
    }
    cerr << endl << "max=" << v[max] << ' ' << max << endl;
*/
//            *****************
  }

  Dir contradir(Dir d) {
    switch (d) {
    case Down: return Up;
    case Up: return Down;
    case Left: return Right;
    case Right: return Left;
    default: return Dir(-1);
    }
  }
  
  void go_weapons(Pos a) {
    Board B;
    queue<Pos> q;
    bool found = false;
    pair<int,Dir> builder;
    bool builderfound = false;

    B[a].dist = 0;
    B[a].vist = true;
    for (Dir d: Directions) {
      if (pos_ok(a+d) and cell(a+d).type == Street and (cell(a+d).b_owner == -1 or cell(a+d).b_owner == me())) {
        B[a+d].dist = 1;
//            ******DEBUG******
/*
        B[a+d].dirs = vector<int> (4,false);
        B[a+d].dirs[d] = true;
*/
//            *****************
        q.push(a+d);
        if (cell(a+d).id != -1) {
          Citizen c = citizen(cell(a+d).id);
          if (c.type == Warrior and c.player == me()) {
            move(c.id, contradir(d));
            found = true;
          }
          else if (c.type == Builder and c.player == me() and not builderfound) {
            builder = make_pair(c.id,contradir(d));
            builderfound = true;
          }
          else if (c.type == Warrior and c.player != me()) {
            if (builderfound) move(builder.first, builder.second);
            found = true;
          }
        }
      }
    }

    while(not q.empty() and not found) {
      Pos b = q.front();
      q.pop();
      for (Dir d : Directions) {
        if (pos_ok(b+d) and cell(b+d).type == Street and (cell(b+d).b_owner == -1 or cell(b+d).b_owner == me())) {
          if (B[b+d].dist == -1) {
            B[b+d].dist = B[b].dist + 1;
//            ******DEBUG******
/*
            B[b+d].dirs = vector<int> (4,false);
            B[b+d].dirs[d] = true;
*/
//            *****************
            q.push(b+d);
            if (cell(b+d).id != -1) {
              Citizen c = citizen(cell(b+d).id);
              if (c.type == Warrior and c.player == me()) {
                move(c.id, contradir(d));
                found = true;
              }
              else if (c.type == Builder and c.player == me() and not builderfound) {
                builder = make_pair(c.id,contradir(d));
                builderfound = true;
              }
              else if (c.type == Warrior and c.player != me()) {
                if (builderfound) move(builder.first, builder.second);
                found = true;
              }
            }
          }
        }
      }
    }
//            ******DEBUG******
/*
    vector<char> aux = {'v','>','^','<'};
    cerr << a << endl;
    for (int i = 0; i < board_rows; ++i) {
      for (int ii = 0; ii < board_cols; ++ii) {
        cerr << '|';
        if (B[Pos(i,ii)].dist >= 0 and B[Pos(i,ii)].dist < 10) cerr << 0;
        cerr << B[Pos(i,ii)].dist;
        if (B[Pos(i,ii)].dist != -1 and B[Pos(i,ii)].dist != 0) {
          for (int d = 0; d < int(B[Pos(i,ii)].dirs.size()); ++d) if(B[Pos(i,ii)].dirs[d]) cerr << aux[d];
        }
        else cerr << '.';
        cerr << '|';
      }
      cerr << endl;
    }
    cerr << endl;
*/
//            *****************
  }

  
  /**
   * Play method, invoked once per each round.
   */
  virtual void play () {
    if (round() == 0) {
      int countdown = num_ini_guns + num_ini_bazookas;
      for (int i = 0; countdown != 0 and i < board_rows; ++i) {
        for (int j = 0; countdown != 0 and j < board_cols; ++j) {
          if (cell(i,j).weapon != NoWeapon) {
            if (cell(i,j).weapon == Gun) weapons[0].insert(Pos(i,j));
            else weapons[1].insert(Pos(i,j));
            --countdown;
          }
        }
      }
    }
    else {
      for (int i = 0; i < 2; ++i) {
        for (auto it = weapons[i].begin(); it != weapons[i].end(); ++it) {
          if (cell(*it).weapon == NoWeapon) {
            weapons[i].erase(it);
            next_weapon.push(round()-1 + num_rounds_regen_weapon);
          }
        }
      }
    }

    if (not next_weapon.empty()) {
      int countdown = 0;
      while (next_weapon.front() == round()) {
        next_weapon.pop();
        ++countdown;
      }
        
      for (int i = 0; countdown != 0 and i < board_rows; ++i) {
        for (int j = 0; countdown != 0 and j < board_cols; ++j) {
          if (cell(i,j).weapon == Gun and weapons[0].insert(Pos(i,j)).second) --countdown;
          if (cell(i,j).weapon == Bazooka and weapons[1].insert(Pos(i,j)).second) --countdown;
        }
      }
    }


    for (Pos p : weapons[1]) go_weapons(p);
    for (Pos p : weapons[0]) go_weapons(p);

    vector<int> w = warriors(me());
    vector<int> b = builders(me());
    for (auto i : w) find(i);
    for (auto i : b) find(i);
  }
};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
